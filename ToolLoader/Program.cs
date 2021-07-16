/*
* 文件资源管理器背景工具扩展
* 
* Author: Maple
* date: 2021-7-14 Create
* Copyright winmoes.com
*/

using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ToolLoader
{
    static class Program
    {
        /// <summary>
        ///  The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            if (args.Count() > 0)
            {
                if (args[0] == "load")
                {
                    LoadDll(true);
                }
                else if (args[0] == "load_s")
                {
                    LoadDll(false);
                }
                else if (args[0] == "reload")
                {
                    Process[] localByName = Process.GetProcessesByName("explorer");
                    for (int i = 0; i < localByName.Length; i++)
                    {
                        if (localByName[i].MainWindowHandle != IntPtr.Zero)
                            localByName[i].Kill();
                    }
                    Process.Start("explorer.exe");
                    System.Threading.Thread.Sleep(1000);
                    LoadDll(true);
                } else if(args[0] == "unload")
                {
                    Process[] localByName = Process.GetProcessesByName("explorer");
                    for (int i = 0; i < localByName.Length; i++)
                    {
                        if (localByName[i].MainWindowHandle != IntPtr.Zero)
                            localByName[i].Kill();
                    }
                    Process.Start("explorer.exe");
                    System.Threading.Thread.Sleep(1000);
                }
            }
        }

        [DllImport("user32.dll")]
        private static extern int EnumWindows(EnumWindowsProc ewp, int lParam);
        [DllImport("user32.dll")]
        public static extern int GetWindowThreadProcessId(IntPtr hwnd, out int ID);
        [DllImport("user32.dll")]
        private static extern int GetClassName(IntPtr hWnd, StringBuilder name, int size);
        [DllImport("user32.dll")]
        private static extern int GetWindowTextLength(IntPtr hWnd);

        public delegate bool EnumWindowsProc(IntPtr hWnd, int lParam);

        static ArrayList ProcList = new ArrayList();
        static int curPID = 0;

        static public bool MEnumWindowsProc(IntPtr hWnd, int lParam)
        {
            int PID = 0;
            GetWindowThreadProcessId(hWnd, out PID);

            //找到与explorer.exe相关的进程窗口
            int curIndex = ProcList.IndexOf(PID);
            if (curIndex != -1)
            {
                //取窗口类名
                int textLen = GetWindowTextLength(hWnd) + 1;
                string title = string.Empty;
                StringBuilder text = new StringBuilder(textLen);
                GetClassName(hWnd, text, textLen);
                title = text.ToString();
                //找到Progman的窗口即是桌面那个explorer进程
                if (title.IndexOf("Progman") != -1)
                    curPID = (int)ProcList[curIndex];
            }
            return true;
        }

        static private void LoadDll(bool tip)
        {
            string curPath = AppDomain.CurrentDomain.SetupInformation.ApplicationBase;
            curPath += "MTweaker.dll";
            if (File.Exists(curPath))
            {
                try
                {
                    //找到explorer进程的所有窗口
                    Process[] localByName = Process.GetProcessesByName("explorer");
                    if (localByName.Length != 0)
                    {
                        ProcList.Clear();
                        for (int i = 0; i < localByName.Length; i++)
                        {
                            ProcList.Add(localByName[i].Id);
                        }
                        curPID = 0;
                        EnumWindowsProc enumProc = new EnumWindowsProc(MEnumWindowsProc);
                        EnumWindows(enumProc, 0);
                        //得到桌面explorer进程ID
                        if (curPID != 0)
                        {
                            //注入
                            EasyHook.RemoteHooking.Inject(curPID, string.Empty, curPath, "Init");
                            if(tip)
                                MessageBox.Show("扩展加载完成.", "加载成功", MessageBoxButtons.OK, MessageBoxIcon.Information);
                        }
                        else
                            MessageBox.Show("Main process not found: explorer.exe", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    else
                        MessageBox.Show("Process not found: explorer.exe", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);

                }
                catch (Exception ExtInfo)
                {
                    MessageBox.Show(ExtInfo.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            else
                MessageBox.Show("File not found: MTweaker.dll", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
}
