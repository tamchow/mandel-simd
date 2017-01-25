using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MandleCS
{
    class Program
    {
        static void Main(string[] args)
        {
            var bmp = DrawMandelbrot((int)(300*(1+2.5)), 600, -2.5, 1, -1, 1);

            PictureBox P = new PictureBox();
            P.Size = bmp.Size;
           
            Form form = new Form
            {
                Name = "Screenshot Displayer",
               
                //Location = new System.Drawing.Point(140, 170),
                Visible = false
                ,AutoSize=true
            };
            form.Size = bmp.Size;
            
            P.Image = bmp;
            form.Controls.Add(P);
            form.ShowDialog();
        }

        public class FastBitmap
        {
            public FastBitmap(int width, int height)
            {
                this.Bitmap = new Bitmap(width, height, PixelFormat.Format24bppRgb);
            }

            public unsafe void SetPixel(int x, int y, Color color)
            {
                BitmapData data = this.Bitmap.LockBits(new Rectangle(0, 0, this.Bitmap.Width, this.Bitmap.Height), ImageLockMode.ReadWrite, PixelFormat.Format24bppRgb);
                IntPtr scan0 = data.Scan0;

                byte* imagePointer = (byte*)scan0.ToPointer(); // Pointer to first pixel of image
                int offset = (y * data.Stride) + (3 * x); // 3x because we have 24bits/px = 3bytes/px
                byte* px = (imagePointer + offset); // pointer to the pixel we want
                px[0] = color.B; // Red component
                px[1] = color.G; // Green component
                px[2] = color.R; // Blue component

                this.Bitmap.UnlockBits(data); // Set the data again
            }

            public Bitmap Bitmap
            {
                get;
                set;
            }
        }

        public static List<Color> GenerateColorPalette()
        {
            List<Color> retVal = new List<Color>();
            for (int i = 0; i <= 255; i++)
            {
                retVal.Add(Color.FromArgb(255, i, i, 255));
            }
            return retVal;
        }

 
        public static Bitmap DrawMandelbrot(int width, int height, double rMin, double rMax, double iMin, double iMax)
        {
            List<Color> Palette = GenerateColorPalette();
            FastBitmap img = new FastBitmap(width, height); // Bitmap to contain the set

            double rScale = (Math.Abs(rMin) + Math.Abs(rMax)) / width; // Amount to move each pixel in the real numbers
            double iScale = (Math.Abs(iMin) + Math.Abs(iMax)) / height; // Amount to move each pixel in the imaginary numbers

            for (int x = 0; x < width; x++)
            {
                for (int y = 0; y < height; y++)
                {
                    Complex c = new Complex(x * rScale + rMin, y * iScale + iMin); // Scaled complex number
                    Complex z = c;
                    for (int i = 0; i < Palette.Count; i++) // 255 iterations with the method we already wrote
                    {
                        if (z.Magnitude >= 2.0)
                        {
                            img.SetPixel(x, y, Palette[i]); // Set the pixel if the magnitude is greater than two
                            break; // We're done with this loop
                        }
                        else
                        {
                            z = c + Complex.Pow(z, 2); // Z = Zlast^2 + C
                        }
                    }
                }
            }

            return img.Bitmap;
        }
    }
}
