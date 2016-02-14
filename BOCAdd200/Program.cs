using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.IO;
using System.Drawing;


namespace BOCAdd200
{
    class Program
    {
        static void Main(string[] args)
        {
            string url = @"https://xcode.eastmoney.com/V2/verifycode2.ashx?vcodeTarget=2203144518925412&rnd=1454655126897";
            string path = @"D:\project data\haha\";
            WebClient myclient = new WebClient();
            int sizepic = 10;
            //下载
            for (int i = 0; i < sizepic; i++)
            {
                string filepath = path + i.ToString() + ".jpg";
                try
                {
                    myclient.DownloadFile(url, filepath);
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                }
            }

            //去色
            for (int i = 0; i < sizepic; i++)
            {
                string filepath = path + i.ToString() + ".jpg";
                string filepath1 = path + @"binary\" + i.ToString() + ".jpg";
                Image img = Image.FromFile(filepath);
                Bitmap bit = new Bitmap(img);
                Bitmap grayBit = toGray(bit, bit.Height, bit.Width);
                grayBit.Save(filepath1);
            }
            //加线
            for (int i = 0; i < sizepic; i++)
            {
                string filepath1 = path + @"binary\" + i.ToString() + ".jpg";
                string filepath2 = path + @"draw\" + i.ToString() + ".jpg";
                Image img = Image.FromFile(filepath1);
                Bitmap bit = new Bitmap(img);
                Bitmap pureBit = fillDraw(bit, bit.Height, bit.Width);
                pureBit.Save(filepath2);
            }
            //去噪
            for (int i = 0; i < sizepic; i++)
            {
                string filepath1 = path + @"draw\" + i.ToString() + ".jpg";
                string filepath2 = path + @"pure\" + i.ToString() + ".jpg";
                Image img = Image.FromFile(filepath1);
                Bitmap bit = new Bitmap(img);
                Bitmap pureBit = purePic(bit, bit.Height, bit.Width);
                Bitmap pure2 = purePic(pureBit, pureBit.Height, pureBit.Width);
                pure2.Save(filepath2);
            }
            //切割
            for (int i = 0; i < sizepic; i++)
            {
                string filepath1 = path + @"pure\" + i.ToString() + ".jpg";
                string filepath2 = path + @"sigle\" + i.ToString();
                Image img = Image.FromFile(filepath1);
                Bitmap bit = new Bitmap(img);
                siglePic(bit, filepath2);
            }
        }

        public static Bitmap toGray(Bitmap pic, int row, int col)
        {
            Color c = new Color();
            int t;

            for (int i = 1; i < col - 2; i++)
            {
                for (int j = 1; j < row - 2; j++)
                {
                    c = pic.GetPixel(i, j);
                    t = (int)((c.R + c.G + c.B) / 3.0);
                    if (t < 200)
                    {
                        t = 0;
                    }
                    else
                    {
                        t = 255;
                    }
                    pic.SetPixel(i, j, Color.FromArgb(t, t, t));
                }
            }
            return pic;
        }

        public static Bitmap fillDraw(Bitmap pic, int row, int col)
        {
            Graphics graph = Graphics.FromImage(pic);
            Pen blackPen = new Pen(Color.Black, 2);
            Point left = new Point(2, 42);
            Point right = new Point(177, 10);
            graph.DrawLine(blackPen, left, right);


            Bitmap rtnPic = new Bitmap(pic);
            return rtnPic;
        }

        public static void siglePic(Bitmap pic, string filepath)
        {
            //來源圖片，大小175 x 61
            int picWidth = 30;
            int picHeight = 61;
            int[] start = { 13, 42, 66, 90, 120 };
            for (int idxX = 0; idxX < 5; idxX++)//X軸迴圈
            {
                Bitmap spic = new Bitmap(picWidth, picHeight);
                //建立圖片
                Graphics graphic = Graphics.FromImage(spic);
                //建立畫板
                graphic.DrawImage(pic,
                    //將被切割的圖片畫在新圖片上面，第一個參數是被切割的原圖片
                         new Rectangle(0, 0, picWidth, picHeight),
                    //指定繪製影像的位置和大小，基本上是同pic大小
                         new Rectangle(start[idxX], 0, picWidth, picHeight),
                    //指定被切割的圖片要繪製的部分
                         GraphicsUnit.Pixel);
                //測量單位，這邊是pixel
                spic.Save(filepath +"_"+ idxX.ToString() + ".jpg");
                graphic.Dispose();
                spic.Dispose();
            }
        }

        public static Bitmap purePic(Bitmap pic, int row, int col)
        {
            int[] colors = new int[9];
            Bitmap rtnPic = new Bitmap(pic);

            for (int i = 1; i < col - 2; i++)
            {
                for (int j = 1; j < row - 2; j++)
                {
                    colors[0] = pic.GetPixel(i, j).R;
                    colors[1] = pic.GetPixel(i - 1, j).R;
                    colors[2] = pic.GetPixel(i, j - 1).R;
                    colors[3] = pic.GetPixel(i + 1, j).R;
                    colors[4] = pic.GetPixel(i, j + 1).R;
                    colors[5] = pic.GetPixel(i - 1, j - 1).R;
                    colors[6] = pic.GetPixel(i + 1, j - 1).R;
                    colors[7] = pic.GetPixel(i + 1, j + 1).R;
                    colors[8] = pic.GetPixel(i - 1, j + 1).R;

                    if (colors[1] + colors[2] + colors[3] + colors[4] + colors[5] + colors[6] + colors[7] + colors[8] > 500)
                    {
                        rtnPic.SetPixel(i, j, Color.FromArgb(255, 255, 255));
                    }

                    //t = (int)((colors[0] + colors[1] + colors[2] + colors[3] + colors[4] + colors[5] + colors[6] + colors[7] + colors[8]) / 9.0);
                    //if (colors[0] > t)
                    //{
                    //    pic.SetPixel(i, j, Color.FromArgb(255, 255, 255));
                    //}
                }
            }
            return rtnPic;
        }
    }
}
