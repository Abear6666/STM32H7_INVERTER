using System.Diagnostics;
using System.Globalization;
using System.IO.Ports;
using System.Text;

namespace ApolloIapSerialGui;

internal sealed class MainForm : Form
{
    private readonly ComboBox _portCombo = new();
    private readonly NumericUpDown _baudNumeric = new();
    private readonly TextBox _fileText = new();
    private readonly TextBox _sizeText = new();
    private readonly TextBox _crcText = new();
    private readonly NumericUpDown _versionNumeric = new();
    private readonly NumericUpDown _chunkNumeric = new();
    private readonly NumericUpDown _delayNumeric = new();
    private readonly CheckBox _waitReadyCheck = new();
    private readonly Button _refreshButton = new();
    private readonly Button _browseButton = new();
    private readonly Button _sendButton = new();
    private readonly Button _cancelButton = new();
    private readonly Button _statusButton = new();
    private readonly Button _clearLogButton = new();
    private readonly Button _saveLogButton = new();
    private readonly ProgressBar _progressBar = new();
    private readonly Label _stageLabel = new();
    private readonly Label _bytesLabel = new();
    private readonly RichTextBox _logBox = new();

    private SerialPort? _monitorPort;
    private CancellationTokenSource? _transferCts;
    private bool _transferActive;

    public MainForm()
    {
        Text = "Apollo H743 串口 IAP 工具";
        MinimumSize = new Size(980, 660);
        StartPosition = FormStartPosition.CenterScreen;
        Font = new Font("Microsoft YaHei UI", 9F, FontStyle.Regular, GraphicsUnit.Point);

        BuildUi();
        WireEvents();
        RefreshPorts();
        ApplyDefaults();
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        _transferCts?.Cancel();
        CloseMonitorPort();
        base.OnFormClosing(e);
    }

    private void BuildUi()
    {
        TableLayoutPanel root = new()
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 2,
            Padding = new Padding(12),
        };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 360));
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        root.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        root.RowStyles.Add(new RowStyle(SizeType.Absolute, 72));
        Controls.Add(root);

        root.Controls.Add(CreateSettingsPanel(), 0, 0);
        root.Controls.Add(CreateLogPanel(), 1, 0);
        root.Controls.Add(CreateProgressPanel(), 0, 1);
        root.SetColumnSpan(root.GetControlFromPosition(0, 1)!, 2);
    }

    private Control CreateSettingsPanel()
    {
        GroupBox group = new()
        {
            Text = "升级设置",
            Dock = DockStyle.Fill,
            Padding = new Padding(12),
        };

        TableLayoutPanel table = new()
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            ColumnCount = 3,
        };
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 86));
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 92));
        group.Controls.Add(table);

        AddLabel(table, "串口", 0);
        _portCombo.DropDownStyle = ComboBoxStyle.DropDownList;
        table.Controls.Add(_portCombo, 1, 0);
        _refreshButton.Text = "刷新";
        table.Controls.Add(_refreshButton, 2, 0);

        AddLabel(table, "波特率", 1);
        _baudNumeric.Maximum = 921600;
        _baudNumeric.Minimum = 1200;
        _baudNumeric.Increment = 9600;
        table.Controls.Add(_baudNumeric, 1, 1);

        AddLabel(table, "固件文件", 2);
        table.Controls.Add(_fileText, 1, 2);
        _browseButton.Text = "选择";
        table.Controls.Add(_browseButton, 2, 2);

        AddLabel(table, "文件大小", 3);
        _sizeText.ReadOnly = true;
        table.Controls.Add(_sizeText, 1, 3);

        AddLabel(table, "CRC32", 4);
        _crcText.ReadOnly = true;
        table.Controls.Add(_crcText, 1, 4);

        AddLabel(table, "版本号", 5);
        _versionNumeric.Maximum = uint.MaxValue;
        _versionNumeric.Minimum = 1;
        table.Controls.Add(_versionNumeric, 1, 5);

        AddLabel(table, "分块字节", 6);
        _chunkNumeric.Maximum = 4096;
        _chunkNumeric.Minimum = 16;
        _chunkNumeric.Increment = 16;
        table.Controls.Add(_chunkNumeric, 1, 6);

        AddLabel(table, "块间延时", 7);
        _delayNumeric.Maximum = 1000;
        _delayNumeric.Minimum = 0;
        table.Controls.Add(_delayNumeric, 1, 7);

        _waitReadyCheck.Text = "发送前等待 App IAP 就绪";
        _waitReadyCheck.AutoSize = true;
        table.Controls.Add(_waitReadyCheck, 1, 8);
        table.SetColumnSpan(_waitReadyCheck, 2);

        FlowLayoutPanel buttons = new()
        {
            FlowDirection = FlowDirection.LeftToRight,
            Dock = DockStyle.Top,
            AutoSize = true,
            Padding = new Padding(0, 18, 0, 0),
        };
        _sendButton.Text = "开始发送";
        _sendButton.Width = 104;
        _cancelButton.Text = "取消";
        _cancelButton.Width = 86;
        _statusButton.Text = "发送 iap status";
        _statusButton.Width = 132;
        buttons.Controls.AddRange(new Control[] { _sendButton, _cancelButton, _statusButton });
        group.Controls.Add(buttons);
        buttons.BringToFront();

        Label note = new()
        {
            Text = "发送完成后包先进入 W25Q staging。复位后 Boot 安装到 AppB 试运行，AppB 启动成功会确认；失败回退 AppA。",
            Dock = DockStyle.Bottom,
            AutoSize = false,
            Height = 64,
            ForeColor = Color.FromArgb(92, 103, 115),
        };
        group.Controls.Add(note);

        return group;
    }

    private Control CreateLogPanel()
    {
        GroupBox group = new()
        {
            Text = "串口日志",
            Dock = DockStyle.Fill,
            Padding = new Padding(10),
        };

        TableLayoutPanel table = new()
        {
            Dock = DockStyle.Fill,
            RowCount = 2,
        };
        table.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        table.RowStyles.Add(new RowStyle(SizeType.Absolute, 42));
        group.Controls.Add(table);

        _logBox.Dock = DockStyle.Fill;
        _logBox.ReadOnly = true;
        _logBox.BackColor = Color.FromArgb(20, 24, 28);
        _logBox.ForeColor = Color.FromArgb(220, 230, 240);
        _logBox.Font = new Font("Consolas", 9.5F);
        _logBox.BorderStyle = BorderStyle.FixedSingle;
        table.Controls.Add(_logBox, 0, 0);

        FlowLayoutPanel buttons = new()
        {
            FlowDirection = FlowDirection.RightToLeft,
            Dock = DockStyle.Fill,
        };
        _clearLogButton.Text = "清空日志";
        _saveLogButton.Text = "保存日志";
        buttons.Controls.AddRange(new Control[] { _clearLogButton, _saveLogButton });
        table.Controls.Add(buttons, 0, 1);

        return group;
    }

    private Control CreateProgressPanel()
    {
        TableLayoutPanel panel = new()
        {
            Dock = DockStyle.Fill,
            ColumnCount = 3,
            RowCount = 2,
            Padding = new Padding(0, 10, 0, 0),
        };
        panel.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 180));
        panel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        panel.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 220));
        panel.RowStyles.Add(new RowStyle(SizeType.Absolute, 24));
        panel.RowStyles.Add(new RowStyle(SizeType.Absolute, 28));

        _stageLabel.Text = "就绪";
        _stageLabel.Dock = DockStyle.Fill;
        panel.Controls.Add(_stageLabel, 0, 0);

        _bytesLabel.Text = "0 / 0";
        _bytesLabel.TextAlign = ContentAlignment.MiddleRight;
        _bytesLabel.Dock = DockStyle.Fill;
        panel.Controls.Add(_bytesLabel, 2, 0);

        _progressBar.Dock = DockStyle.Fill;
        _progressBar.Minimum = 0;
        _progressBar.Maximum = 1000;
        panel.Controls.Add(_progressBar, 0, 1);
        panel.SetColumnSpan(_progressBar, 3);

        return panel;
    }

    private static void AddLabel(TableLayoutPanel table, string text, int row)
    {
        table.RowStyles.Add(new RowStyle(SizeType.Absolute, 34));
        Label label = new()
        {
            Text = text,
            Dock = DockStyle.Fill,
            TextAlign = ContentAlignment.MiddleLeft,
        };
        table.Controls.Add(label, 0, row);
    }

    private void WireEvents()
    {
        _refreshButton.Click += (_, _) => RefreshPorts();
        _browseButton.Click += (_, _) => BrowseFile();
        _sendButton.Click += async (_, _) => await StartTransferAsync();
        _cancelButton.Click += (_, _) => _transferCts?.Cancel();
        _statusButton.Click += (_, _) => SendStatusCommand();
        _clearLogButton.Click += (_, _) => _logBox.Clear();
        _saveLogButton.Click += (_, _) => SaveLog();
        _fileText.TextChanged += (_, _) => UpdateFileInfo();
    }

    private void ApplyDefaults()
    {
        _baudNumeric.Value = 115200;
        _versionNumeric.Value = 2;
        _chunkNumeric.Value = 128;
        _delayNumeric.Value = 2;
        _waitReadyCheck.Checked = true;
        _cancelButton.Enabled = false;

        string defaultBin = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory,
            "..", "..", "..", "..", "build", "gcc-debug", "app_b_slot.bin"));
        if (File.Exists(defaultBin))
        {
            _fileText.Text = defaultBin;
        }
    }

    private void RefreshPorts()
    {
        string? selected = _portCombo.SelectedItem as string;
        _portCombo.Items.Clear();
        foreach (string portName in SerialPort.GetPortNames().OrderBy(static p => p))
        {
            _portCombo.Items.Add(portName);
        }

        if (selected is not null && _portCombo.Items.Contains(selected))
        {
            _portCombo.SelectedItem = selected;
        }
        else if (_portCombo.Items.Count > 0)
        {
            _portCombo.SelectedIndex = 0;
        }
    }

    private void BrowseFile()
    {
        using OpenFileDialog dialog = new()
        {
            Filter = "Binary files (*.bin)|*.bin|All files (*.*)|*.*",
            Title = "选择升级 bin 文件",
        };

        if (File.Exists(_fileText.Text))
        {
            dialog.InitialDirectory = Path.GetDirectoryName(_fileText.Text);
            dialog.FileName = Path.GetFileName(_fileText.Text);
        }

        if (dialog.ShowDialog(this) == DialogResult.OK)
        {
            _fileText.Text = dialog.FileName;
        }
    }

    private void UpdateFileInfo()
    {
        string path = _fileText.Text.Trim();
        if (!File.Exists(path))
        {
            _sizeText.Text = "";
            _crcText.Text = "";
            return;
        }

        try
        {
            FileInfo info = new(path);
            uint crc32 = Crc32.ComputeFile(path);
            _sizeText.Text = info.Length.ToString(CultureInfo.InvariantCulture);
            _crcText.Text = $"0x{crc32:X8}";
        }
        catch (Exception ex)
        {
            _sizeText.Text = "";
            _crcText.Text = "";
            AppendLog($"文件读取失败：{ex.Message}\r\n");
        }
    }

    private async Task StartTransferAsync()
    {
        if (_transferActive)
        {
            return;
        }

        try
        {
            CloseMonitorPort();
            _transferActive = true;
            SetBusyState(true);
            _transferCts = new CancellationTokenSource();

            IapTransferOptions options = new()
            {
                PortName = GetSelectedPort(),
                BaudRate = (int)_baudNumeric.Value,
                FilePath = _fileText.Text.Trim(),
                Version = (uint)_versionNumeric.Value,
                ChunkSize = (int)_chunkNumeric.Value,
                ChunkDelayMs = (int)_delayNumeric.Value,
                WaitForAppReady = _waitReadyCheck.Checked,
            };

            SerialIapTransfer transfer = new(AppendLog, UpdateProgress);
            await transfer.RunAsync(options, _transferCts.Token);
        }
        catch (OperationCanceledException)
        {
            AppendLog("\r\n用户取消\r\n");
            UpdateProgress(new IapTransferProgress { Stage = "已取消", SentBytes = 0, TotalBytes = 0 });
        }
        catch (Exception ex)
        {
            AppendLog($"\r\n错误：{ex.Message}\r\n");
            UpdateProgress(new IapTransferProgress { Stage = "失败", SentBytes = 0, TotalBytes = 0 });
        }
        finally
        {
            _transferCts?.Dispose();
            _transferCts = null;
            _transferActive = false;
            SetBusyState(false);
        }
    }

    private void SendStatusCommand()
    {
        try
        {
            SerialPort port = EnsureMonitorPort();
            byte[] data = Encoding.ASCII.GetBytes("iap status\n");
            port.Write(data, 0, data.Length);
            AppendLog("iap status\r\n");
        }
        catch (Exception ex)
        {
            AppendLog($"发送 iap status 失败：{ex.Message}\r\n");
        }
    }

    private SerialPort EnsureMonitorPort()
    {
        if (_monitorPort is { IsOpen: true })
        {
            return _monitorPort;
        }

        _monitorPort = new SerialPort(GetSelectedPort(), (int)_baudNumeric.Value, Parity.None, 8, StopBits.One)
        {
            Handshake = Handshake.None,
            DtrEnable = false,
            RtsEnable = false,
            Encoding = Encoding.UTF8,
        };
        _monitorPort.DataReceived += MonitorPortOnDataReceived;
        _monitorPort.Open();
        _monitorPort.DiscardInBuffer();
        AppendLog($"串口监听已打开：{_monitorPort.PortName}\r\n");
        return _monitorPort;
    }

    private void MonitorPortOnDataReceived(object sender, SerialDataReceivedEventArgs e)
    {
        try
        {
            if (sender is not SerialPort port)
            {
                return;
            }

            string text = port.ReadExisting();
            if (text.Length > 0)
            {
                AppendLog(text);
            }
        }
        catch
        {
            // DataReceived may race with port close; ignore that shutdown path.
        }
    }

    private void CloseMonitorPort()
    {
        if (_monitorPort is null)
        {
            return;
        }

        try
        {
            _monitorPort.DataReceived -= MonitorPortOnDataReceived;
            if (_monitorPort.IsOpen)
            {
                _monitorPort.Close();
            }
        }
        finally
        {
            _monitorPort.Dispose();
            _monitorPort = null;
        }
    }

    private string GetSelectedPort()
    {
        if (_portCombo.SelectedItem is string portName && portName.Length > 0)
        {
            return portName;
        }

        throw new InvalidOperationException("请选择串口");
    }

    private void SetBusyState(bool busy)
    {
        _sendButton.Enabled = !busy;
        _cancelButton.Enabled = busy;
        _refreshButton.Enabled = !busy;
        _browseButton.Enabled = !busy;
        _statusButton.Enabled = !busy;
        _portCombo.Enabled = !busy;
        _baudNumeric.Enabled = !busy;
        _fileText.Enabled = !busy;
        _versionNumeric.Enabled = !busy;
        _chunkNumeric.Enabled = !busy;
        _delayNumeric.Enabled = !busy;
        _waitReadyCheck.Enabled = !busy;
    }

    private void UpdateProgress(IapTransferProgress progress)
    {
        if (InvokeRequired)
        {
            BeginInvoke(() => UpdateProgress(progress));
            return;
        }

        _stageLabel.Text = progress.Stage;
        if (progress.TotalBytes > 0)
        {
            _bytesLabel.Text = $"{progress.SentBytes} / {progress.TotalBytes} ({progress.Percent:F1}%)";
            int value = (int)Math.Clamp(progress.Percent * 10.0, 0.0, 1000.0);
            _progressBar.Value = value;
        }
        else
        {
            _bytesLabel.Text = "0 / 0";
            _progressBar.Value = 0;
        }
    }

    private void AppendLog(string text)
    {
        if (InvokeRequired)
        {
            BeginInvoke(() => AppendLog(text));
            return;
        }

        int start = _logBox.TextLength;
        _logBox.AppendText(text);
        int length = _logBox.TextLength - start;

        if (length > 0)
        {
            Color color = PickLogColor(text);
            _logBox.Select(start, length);
            _logBox.SelectionColor = color;
            _logBox.Select(_logBox.TextLength, 0);
        }

        _logBox.ScrollToCaret();
    }

    private static Color PickLogColor(string text)
    {
        if (text.Contains("failed", StringComparison.OrdinalIgnoreCase) ||
            text.Contains("fail", StringComparison.OrdinalIgnoreCase) ||
            text.Contains("abort", StringComparison.OrdinalIgnoreCase) ||
            text.Contains("错误", StringComparison.OrdinalIgnoreCase))
        {
            return Color.FromArgb(255, 125, 125);
        }

        if (text.Contains("ready", StringComparison.OrdinalIgnoreCase) ||
            text.Contains("verified", StringComparison.OrdinalIgnoreCase) ||
            text.Contains("pending set", StringComparison.OrdinalIgnoreCase) ||
            text.Contains("CRC OK", StringComparison.OrdinalIgnoreCase))
        {
            return Color.FromArgb(125, 230, 155);
        }

        return Color.FromArgb(220, 230, 240);
    }

    private void SaveLog()
    {
        using SaveFileDialog dialog = new()
        {
            Filter = "Text files (*.txt)|*.txt|All files (*.*)|*.*",
            FileName = $"iap_log_{DateTime.Now:yyyyMMdd_HHmmss}.txt",
        };

        if (dialog.ShowDialog(this) == DialogResult.OK)
        {
            File.WriteAllText(dialog.FileName, _logBox.Text, Encoding.UTF8);
        }
    }
}
