using System.Diagnostics;
using System.IO.Ports;
using System.Text;

namespace ApolloIapSerialGui;

internal sealed class IapTransferOptions
{
    public required string PortName { get; init; }
    public int BaudRate { get; init; } = 115200;
    public required string FilePath { get; init; }
    public uint Version { get; init; } = 2U;
    public int ChunkSize { get; init; } = 128;
    public int ChunkDelayMs { get; init; } = 2;
    public int AppTimeoutMs { get; init; } = 60_000;
    public int ReadyTimeoutMs { get; init; } = 20_000;
    public int LogTailMs { get; init; } = 8_000;
    public bool WaitForAppReady { get; init; } = true;
}

internal sealed class IapTransferProgress
{
    public required string Stage { get; init; }
    public long SentBytes { get; init; }
    public long TotalBytes { get; init; }
    public double Percent => TotalBytes <= 0 ? 0.0 : SentBytes * 100.0 / TotalBytes;
}

internal sealed class SerialIapTransfer
{
    private static readonly string[] AppReadyMarkers =
    {
        "task_iap started",
        "IAP status:",
        "iap status:",
    };

    private readonly Action<string> _log;
    private readonly Action<IapTransferProgress> _progress;
    private readonly StringBuilder _recentText = new();
    private readonly object _sync = new();

    public SerialIapTransfer(Action<string> log, Action<IapTransferProgress> progress)
    {
        _log = log;
        _progress = progress;
    }

    public async Task RunAsync(IapTransferOptions options, CancellationToken cancellationToken)
    {
        ValidateOptions(options);

        FileInfo fileInfo = new(options.FilePath);
        uint crc32 = Crc32.ComputeFile(options.FilePath);
        string command = $"iap recv {fileInfo.Length} 0x{crc32:X8} {options.Version}\r\n";

        _log($"serial={options.PortName} baud={options.BaudRate}\r\n");
        _log($"file={options.FilePath} size={fileInfo.Length} crc32=0x{crc32:X8} version={options.Version}\r\n");

        using SerialPort port = new(options.PortName, options.BaudRate, Parity.None, 8, StopBits.One)
        {
            Handshake = Handshake.None,
            DtrEnable = false,
            RtsEnable = false,
            ReadTimeout = 50,
            WriteTimeout = 5000,
            Encoding = Encoding.ASCII,
        };

        port.Open();
        port.DiscardInBuffer();
        port.DiscardOutBuffer();
        _progress(new IapTransferProgress { Stage = "串口已打开", SentBytes = 0, TotalBytes = fileInfo.Length });

        if (options.WaitForAppReady)
        {
            await WaitForAnyAsync(port, AppReadyMarkers, options.AppTimeoutMs, "等待 App IAP 就绪", cancellationToken);
        }

        _progress(new IapTransferProgress { Stage = "发送接收命令", SentBytes = 0, TotalBytes = fileInfo.Length });
        WriteAscii(port, command);
        _log(command.TrimEnd() + "\r\n");

        await WaitForAnyAsync(port,
                              new[] { "ready for binary" },
                              options.ReadyTimeoutMs,
                              "等待板端 ready for binary",
                              cancellationToken);

        await SendFileAsync(port, options, fileInfo.Length, cancellationToken);
        _log($"\r\nsent={fileInfo.Length}\r\n");

        _progress(new IapTransferProgress { Stage = "等待板端校验日志", SentBytes = fileInfo.Length, TotalBytes = fileInfo.Length });
        await DrainLogsAsync(port, options.LogTailMs, cancellationToken);
        _progress(new IapTransferProgress { Stage = "发送完成", SentBytes = fileInfo.Length, TotalBytes = fileInfo.Length });
    }

    private async Task SendFileAsync(SerialPort port,
                                     IapTransferOptions options,
                                     long totalBytes,
                                     CancellationToken cancellationToken)
    {
        byte[] buffer = new byte[options.ChunkSize];
        long sent = 0;

        _progress(new IapTransferProgress { Stage = "发送二进制包", SentBytes = sent, TotalBytes = totalBytes });
        using FileStream stream = File.OpenRead(options.FilePath);
        while (true)
        {
            int count = await stream.ReadAsync(buffer.AsMemory(0, buffer.Length), cancellationToken);
            if (count <= 0)
            {
                break;
            }

            cancellationToken.ThrowIfCancellationRequested();
            port.Write(buffer, 0, count);
            sent += count;
            _progress(new IapTransferProgress { Stage = "发送二进制包", SentBytes = sent, TotalBytes = totalBytes });

            if (options.ChunkDelayMs > 0)
            {
                await Task.Delay(options.ChunkDelayMs, cancellationToken);
            }

            ReadAvailable(port);
        }
    }

    private async Task WaitForAnyAsync(SerialPort port,
                                       IReadOnlyCollection<string> markers,
                                       int timeoutMs,
                                       string stage,
                                       CancellationToken cancellationToken)
    {
        Stopwatch stopwatch = Stopwatch.StartNew();
        _progress(new IapTransferProgress { Stage = stage, SentBytes = 0, TotalBytes = 0 });

        while (stopwatch.ElapsedMilliseconds < timeoutMs)
        {
            cancellationToken.ThrowIfCancellationRequested();
            ReadAvailable(port);

            string snapshot;
            lock (_sync)
            {
                snapshot = _recentText.ToString();
            }

            foreach (string marker in markers)
            {
                if (snapshot.Contains(marker, StringComparison.OrdinalIgnoreCase))
                {
                    return;
                }
            }

            await Task.Delay(20, cancellationToken);
        }

        throw new TimeoutException($"{stage} 超时");
    }

    private async Task DrainLogsAsync(SerialPort port, int tailMs, CancellationToken cancellationToken)
    {
        Stopwatch stopwatch = Stopwatch.StartNew();
        while (stopwatch.ElapsedMilliseconds < tailMs)
        {
            cancellationToken.ThrowIfCancellationRequested();
            ReadAvailable(port);
            await Task.Delay(20, cancellationToken);
        }
    }

    private void ReadAvailable(SerialPort port)
    {
        int bytesToRead = port.BytesToRead;
        if (bytesToRead <= 0)
        {
            return;
        }

        byte[] buffer = new byte[bytesToRead];
        int count = port.Read(buffer, 0, buffer.Length);
        if (count <= 0)
        {
            return;
        }

        string text = Encoding.UTF8.GetString(buffer, 0, count);
        _log(text);

        lock (_sync)
        {
            _recentText.Append(text);
            if (_recentText.Length > 8192)
            {
                _recentText.Remove(0, _recentText.Length - 8192);
            }
        }
    }

    private static void WriteAscii(SerialPort port, string text)
    {
        byte[] bytes = Encoding.ASCII.GetBytes(text);
        port.Write(bytes, 0, bytes.Length);
    }

    private static void ValidateOptions(IapTransferOptions options)
    {
        if (string.IsNullOrWhiteSpace(options.PortName))
        {
            throw new InvalidOperationException("请选择串口");
        }

        if (!File.Exists(options.FilePath))
        {
            throw new InvalidOperationException("请选择有效的 bin 文件");
        }

        if (new FileInfo(options.FilePath).Length <= 0)
        {
            throw new InvalidOperationException("bin 文件为空");
        }

        if (options.Version == 0U)
        {
            throw new InvalidOperationException("版本号不能为 0");
        }

        if (options.ChunkSize <= 0)
        {
            throw new InvalidOperationException("分块大小必须大于 0");
        }
    }
}
