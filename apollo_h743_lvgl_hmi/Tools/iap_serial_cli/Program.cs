using ApolloIapSerialGui;

IapTransferProgress? lastPrintedProgress = null;

static int ParseInt(string text)
{
    return int.Parse(text, System.Globalization.CultureInfo.InvariantCulture);
}

static uint ParseUInt32(string text)
{
    return text.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
        ? Convert.ToUInt32(text[2..], 16)
        : uint.Parse(text, System.Globalization.CultureInfo.InvariantCulture);
}

static string? ValueAfter(string[] args, string name)
{
    for (int i = 0; i < args.Length - 1; i++)
    {
        if (args[i] == name)
        {
            return args[i + 1];
        }
    }

    return null;
}

static bool HasFlag(string[] args, string name)
{
    return args.Any(arg => arg == name);
}

static void PrintUsage()
{
    Console.WriteLine("ApolloIapSerialCli --port COM5 --file build/gcc-debug/app_b_slot.bin [--baud 115200] [--version 2]");
    Console.WriteLine("Options: --chunk 128 --delay-ms 2 --app-timeout-ms 60000 --ready-timeout-ms 20000 --log-tail-ms 8000 --no-wait-app");
}

if (HasFlag(args, "--help") || HasFlag(args, "-h"))
{
    PrintUsage();
    return 0;
}

string? port = ValueAfter(args, "--port");
string? file = ValueAfter(args, "--file");
if (string.IsNullOrWhiteSpace(port) || string.IsNullOrWhiteSpace(file))
{
    PrintUsage();
    return 2;
}

IapTransferOptions options = new()
{
    PortName = port,
    FilePath = file,
    BaudRate = ValueAfter(args, "--baud") is { } baud ? ParseInt(baud) : 115200,
    Version = ValueAfter(args, "--version") is { } version ? ParseUInt32(version) : 2U,
    ChunkSize = ValueAfter(args, "--chunk") is { } chunk ? ParseInt(chunk) : 128,
    ChunkDelayMs = ValueAfter(args, "--delay-ms") is { } delay ? ParseInt(delay) : 2,
    AppTimeoutMs = ValueAfter(args, "--app-timeout-ms") is { } appTimeout ? ParseInt(appTimeout) : 60_000,
    ReadyTimeoutMs = ValueAfter(args, "--ready-timeout-ms") is { } readyTimeout ? ParseInt(readyTimeout) : 20_000,
    LogTailMs = ValueAfter(args, "--log-tail-ms") is { } logTail ? ParseInt(logTail) : 8_000,
    WaitForAppReady = !HasFlag(args, "--no-wait-app"),
};

using CancellationTokenSource cts = new();
Console.CancelKeyPress += (_, eventArgs) =>
{
    eventArgs.Cancel = true;
    cts.Cancel();
};

SerialIapTransfer transfer = new(Console.Write, progress =>
{
    if (progress.TotalBytes > 0)
    {
        bool shouldPrint = lastPrintedProgress is null ||
                           progress.Stage != lastPrintedProgress.Stage ||
                           progress.SentBytes == progress.TotalBytes ||
                           progress.SentBytes - lastPrintedProgress.SentBytes >= 16 * 1024;

        if (shouldPrint)
        {
            Console.Error.WriteLine($"{progress.Stage}: {progress.SentBytes}/{progress.TotalBytes} {progress.Percent:F1}%");
            lastPrintedProgress = progress;
        }
    }
    else
    {
        Console.Error.WriteLine(progress.Stage);
        lastPrintedProgress = progress;
    }
});

try
{
    await transfer.RunAsync(options, cts.Token);
    Console.Error.WriteLine();
    return 0;
}
catch (OperationCanceledException)
{
    Console.Error.WriteLine("已取消");
    return 130;
}
catch (Exception ex)
{
    Console.Error.WriteLine($"错误：{ex.Message}");
    return 1;
}
