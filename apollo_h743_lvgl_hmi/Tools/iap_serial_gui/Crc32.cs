namespace ApolloIapSerialGui;

internal static class Crc32
{
    private const uint Polynomial = 0xEDB88320U;

    public static uint Compute(ReadOnlySpan<byte> data)
    {
        return Update(0U, data);
    }

    public static uint ComputeFile(string path)
    {
        uint crc = 0U;
        byte[] buffer = new byte[64 * 1024];

        using FileStream stream = File.OpenRead(path);
        while (true)
        {
            int count = stream.Read(buffer, 0, buffer.Length);
            if (count <= 0)
            {
                break;
            }

            crc = Update(crc, buffer.AsSpan(0, count));
        }

        return crc;
    }

    public static uint Update(uint crc, ReadOnlySpan<byte> data)
    {
        crc = ~crc;
        foreach (byte value in data)
        {
            crc ^= value;
            for (int bit = 0; bit < 8; bit++)
            {
                uint mask = 0U - (crc & 1U);
                crc = (crc >> 1) ^ (Polynomial & mask);
            }
        }

        return ~crc;
    }
}
