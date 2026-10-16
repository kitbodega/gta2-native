using System;
using System.IO;
using System.IO.Compression;

// mkintro <in.raw> <out.mv> <w> <h> <frames> <fps> <delayMs>
// Convierte el .raw de binkdecode (deltas crudos por frame) a intro.mv
// (G2MV v2): header 64B + por frame u32 csize + DEFLATE(delta[w*h*2]).
class MkIntro
{
    static void PutU32(byte[] a, int off, uint v)
    {
        a[off] = (byte)(v & 0xFF);
        a[off + 1] = (byte)((v >> 8) & 0xFF);
        a[off + 2] = (byte)((v >> 16) & 0xFF);
        a[off + 3] = (byte)((v >> 24) & 0xFF);
    }

    static int Main(string[] args)
    {
        if (args.Length != 7)
        {
            Console.Error.WriteLine("uso: mkintro <in.raw> <out.mv> <w> <h> <frames> <fps> <delayMs>");
            return 2;
        }
        string rin = args[0], rout = args[1];
        int w = int.Parse(args[2]), h = int.Parse(args[3]);
        int frames = int.Parse(args[4]), fps = int.Parse(args[5]);
        int delay = int.Parse(args[6]);
        if (w <= 0 || h <= 0 || w > 4096 || h > 4096 || frames <= 0 || frames > 100000)
        {
            Console.Error.WriteLine("parametros invalidos");
            return 2;
        }
        int nbytes = w * h * 2;
        byte[] hd = new byte[64];
        hd[0] = (byte)'G'; hd[1] = (byte)'2'; hd[2] = (byte)'M'; hd[3] = (byte)'V';
        hd[4] = 2;              // ver
        hd[5] = 1;              // pixfmt RGB565
        PutU32(hd, 8, (uint)w);
        PutU32(hd, 12, (uint)h);
        PutU32(hd, 16, (uint)frames);
        PutU32(hd, 20, (uint)fps);
        PutU32(hd, 24, 0);      // dataSize (se parchea al final)
        PutU32(hd, 28, (uint)delay); // pad2[0..3]

        byte[] src = new byte[nbytes];

        Console.WriteLine("mkintro: {0}x{1} frames={2} -> {3}", w, h, frames, rout);
        long total = 0;
        using (var fs = File.Create(rout))
        {
            fs.Write(hd, 0, 64);
            using (var fr = File.OpenRead(rin))
            {
                int done = 0;
                for (int i = 0; i < frames; i++)
                {
                    int got = 0;
                    while (got < nbytes)
                    {
                        int r = fr.Read(src, got, nbytes - got);
                        if (r <= 0) break;
                        got += r;
                    }
                    if (got < nbytes)
                    {
                        Console.Error.WriteLine("advertencia: raw corto en frame {0} ({1}/{2})", i, got, nbytes);
                        break;
                    }

                    byte[] comp;
                    using (var ms = new MemoryStream())
                    using (var ds = new DeflateStream(ms, CompressionLevel.Optimal, true))
                    {
                        ds.Write(src, 0, nbytes);
                        ds.Close(); // descarga el buffer interno -> stream completo
                        comp = ms.ToArray();
                    }
                    int len = comp.Length;

                    fs.WriteByte((byte)(len & 0xFF));
                    fs.WriteByte((byte)((len >> 8) & 0xFF));
                    fs.WriteByte((byte)((len >> 16) & 0xFF));
                    fs.WriteByte((byte)((len >> 24) & 0xFF));
                    fs.Write(comp, 0, len);
                    total += 4 + len;
                    done = i + 1;
                }
                Console.Error.WriteLine("frames procesados: {0}", done);
            }
            fs.Seek(24, SeekOrigin.Begin);
            PutU32(hd, 24, (uint)total);
            fs.Write(hd, 24, 4);
        }
        Console.WriteLine("listo: {0} bytes ({1:0.0} MB)", total, total / 1048576.0);
        return 0;
    }
}