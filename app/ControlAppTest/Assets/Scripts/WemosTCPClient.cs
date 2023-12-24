using Proyecto26;
using RSG;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
#if UNITY_EDITOR
using UnityEditor;
#endif
using UnityEngine;


class NETClient
{
    public class NETResponseHelper
    {
        public string Text="";
    }

    TcpClient tcpClient;

    NetworkStream stream;

    string netBuffer = "";

    List<string> receiveBuffer = new List<string>();

    public event System.Action<string> OnReceive;

    int messageID = 0;

    Dictionary<string, Action<NETResponseHelper>> callMap = new Dictionary<string, Action<NETResponseHelper>>();
    public NETClient()
    {
    }

    public void Start(string ipAddress, int port)
    {
      //  var endPoint = new IPEndPoint(IPAddress.Parse(ipAddress), port);
        tcpClient = new TcpClient();
      
        tcpClient.Connect(ipAddress, port);

        stream = tcpClient.GetStream();
        stream.ReadTimeout = 2000;

        if (tcpClient.Connected)
            Send("OK");
        Update();
    }

    int cc = 0;
    public void Update()
    {
        try
        {
            if (stream.DataAvailable)
            {
                var buffer = new byte[4096];
                int byteCount = stream.Read(buffer, 0, buffer.Length);

                if (byteCount > 0)
                {
                    for (int i = 0; i < byteCount; i++)
                    {
                        if ((char)buffer[i] == '\n')
                        {
                            if (netBuffer != "ACK")
                            {
                                OnReceive?.Invoke(netBuffer);

                                if (netBuffer[0] == '>')
                                {
                                    int idx = netBuffer.IndexOf(".");
                                    if (idx != -1)
                                    {
                                        receiveBuffer.Add(netBuffer);

                                        string id = netBuffer.Substring(1, idx - 1);
                                        string msg = netBuffer.Substring(idx + 1);
                                        Debug.Log("MAP: " + id + " " + msg);
                                        if (callMap.ContainsKey(id))
                                        {
                                            var promise = callMap[id];
                                            callMap.Remove(id);

                                            promise.Invoke(new NETResponseHelper() { Text = msg });
                                            //  promise.Resolve(new NETResponseHelper() { Text = msg });
                                        }
                                        else
                                            Debug.LogError("bad request" + netBuffer);
                                    }
                                    else
                                        Debug.LogError("bad request" + netBuffer);
                                }
                            }
                            netBuffer = "";
                        }
                        else
                            netBuffer += (char)buffer[i];
                    }
                    //  var response = Encoding.UTF8.GetString(buffer, 0, byteCount);

                }
            }
        }
        catch (System.Exception e)
        {
            Debug.LogError(e);
        }

    }

    public void UpdateSlow()
    {
       // Debug.Log("NET STATUS" + tcpClient.Connected);
        if (tcpClient.Connected)
            Send("/ping/"+(cc++));
    }
   
    public void Send(string message)
    {
        lock (this)
        {
            Debug.Log("SEND :" + message);
            byte[] bytes = Encoding.UTF8.GetBytes(message + "\n");
            stream.Write(bytes);
        }
    }
    public IPromise<NETResponseHelper> Get(string url)
    {
        return Get(new RequestHelper { Uri = url });
    }
    public IPromise<NETResponseHelper> Get(RequestHelper options)
    {
        lock (this)
        {
            var promise = new Promise<NETResponseHelper>();
            callMap.Add("" + messageID,(res)=>
            {
                promise.Resolve(res);
            });
            string msg = "" + messageID + "." + options.Uri;
            messageID++;
            Send(msg);
            //  Get(options, promise.Promisify1);
            return promise;
        }
    }
    //public void Get(RequestHelper options, System.Action<RequestException, ResponseHelper> callback)
    //{
    //    string msg = options.Uri;
    //    Send(msg);
    //    //string msg = options-;
    //    //Send
    //    //  options.Method = UnityWebRequest.kHttpVerbGET;
    //    //  Request(options, callback);
    //}
}

[System.Serializable]
public class WemosTCPClient
{
    public string boardAddress = "http://192.168.0.23";
    public int port = 8888;
    //private RequestHelper currentRequest;
    NETClient netClient;

    public string currentActivity="";

    public bool debugMode = false;

    public void Start()
    {
        netClient = new NETClient();
        netClient.Start(boardAddress, port);
        netClient.OnReceive += NetClient_OnReceive;
    }

    private void NetClient_OnReceive(string message)
    {
        Debug.Log("<< " + message);
    }

    public void Update()
    {
        netClient.Update();
    }
    public void UpdateSlow()
    {
        netClient.UpdateSlow();
    }
    private void LogMessage(string title, string message,bool isError=false)
    {
        if (!debugMode) return;

        currentActivity = title+" " + message;
        if (isError) currentActivity = "ERROR:" + message;
        //#if UNITY_EDITOR
        //        EditorUtility.DisplayDialog(title, message, "Ok");
        //#else
        if (isError)
            Debug.LogError(currentActivity);
        else
            Debug.Log(currentActivity);
        //#endif
    }

    public void GetDump(System.Action<RobotState> onState) 
    {
        LogMessage("GetDump", "Calling");
        netClient.Get("/dump").Then(res =>
        {
            LogMessage("GetDump", res.Text);
            //   SumoState r = JsonUtility.FromJson<SumoState>(res.Text);
            RobotState r = JsonUtility.FromJson<RobotState>(res.Text);
            onState.Invoke(r);
            // LogMessage("GetState", "success:" + r.success);
        }).Catch(err =>
        {
            this.LogMessage("GetDump", err.Message, true);
            onState.Invoke(null);
        });
    }

 
    public void SetMotor(bool isLeftMotor,int speed)
    {
        WriteVar((isLeftMotor) ? "motors.left" : "motors.right", speed);
    }

    public void SetMotorBreak(bool breakMode)
    {
        WriteVar("mode.breakMode", breakMode);
    }

    public void SetGuiding(int speed,int steering)
    {
        LogMessage("SetGuiding", "Calling");
        netClient.Get(boardAddress + "/drive?speed=" + speed+ "&steering="+ steering
           ).Then(res =>
            {
            }).Catch(err => this.LogMessage("SetGuiding", err.Message, true));
    }
    // ===============

    string formatVal(object o)
    {
        if (o is bool)
            return ((bool)o).ToString().ToLower();
        else return o.ToString();
    }
    string typeVal(object o)
    {
        if (o is bool) return "B";
        else if (o is int) return "I";
        else if (o is float) return "F";
        else return "D";
    }
    /// <summary>
    /// pars = coppie nome, valore
    /// </summary>
    /// <param name="name"></param>
    /// <param name="prs"></param>
    public void CommandGET(string name,params object[] pars)
    {
        LogMessage("CommandGET", "Calling");
        string p = "";
        for (int i = 0; i < pars.Length; i+=2)
             p += pars[i] + "=" + formatVal( pars[i + 1]);

        netClient.Get("/"+name+ (( p.Length>0) ? "?"+p:"")).Then(res =>
        {
            //  PingResponse r = JsonUtility.FromJson<PingResponse>(res.Text);
        }).Catch(err => this.LogMessage("CommandGET", err.Message, true));
    }

    public void WriteVar<T>(string name,T val )
    {
        LogMessage("WriteVar", "Calling");
        string p = "?name="+ name+"&val="+formatVal(val)+"&type="+ typeVal(val);
        netClient.Get("/write"+p).Then(res =>
        {
        }).Catch(err => this.LogMessage("WriteVar", err.Message, true));
    }
}
