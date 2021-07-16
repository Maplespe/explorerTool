/*
* 文件资源管理器背景工具扩展
* 
* Author: Maple
* date: 2021-7-14 Create
* Copyright winmoes.com
*/

using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;
using EasyHook;

namespace MTweaker
{
    public class Class1 : IEntryPoint
    {
        [DllImport("ExplorerBgTool.dll")]
        public static extern bool InjectionEntryPoint();

        public Class1(RemoteHooking.IContext InContext, String InArg1)
        {

        }

        bool IsInject = false;
        string dllName = string.Empty;

       public void Run(RemoteHooking.IContext InContext, String InArg1)
        {
            string curPath = Path.GetDirectoryName(new Uri(this.GetType().Assembly.CodeBase).AbsolutePath);
            dllName = curPath + "\\MTweaker.dll";
            curPath += "\\ExplorerBgTool.dll";

            //开始调用c++Hook
            InjectionEntryPoint();

            //先检查核心文件是否存在
            if (File.Exists(curPath))
            {
                //第一次注入 等待注入文件资源管理器普通线程
                if(InArg1 == "Init")
                {
                    while (!IsInject)
                    {
                        Process[] localByName = Process.GetProcessesByName("explorer");
                        for (int i = 0; i < localByName.Length; i++)
                        {
                            if (Process.GetCurrentProcess().Id != localByName[i].Id)
                            {
                                if (localByName[i].MainWindowHandle != IntPtr.Zero)
                                {
                                    //MessageBox.Show("已注入到主框架 :" + localByName[i].MainWindowTitle);
                                    try
                                    {
                                        RemoteHooking.Inject(localByName[i].Id, string.Empty, dllName, string.Empty);
                                    }
                                    catch
                                    {

                                    }
                                    IsInject = true;
                                    break;
                                }
                            }
                        }
                        Thread.Sleep(1500);
                    }
                }
            } else
                MessageBox.Show("File not found: ExplorerBgTool.dll\n" + curPath, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
}
