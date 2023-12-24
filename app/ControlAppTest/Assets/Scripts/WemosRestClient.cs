using Proyecto26;
using System.Collections;
using System.Collections.Generic;
#if UNITY_EDITOR
using UnityEditor;
#endif
using UnityEngine;

[System.Serializable]
public class WemosRestClient 
{
    public string boardAddress = "http://192.168.0.23";
    private RequestHelper currentRequest;

    public string currentActivity="";

    public bool debugMode = false;

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

    public void Ping()
    {  
        // We can add default request headers for all requests
        RestClient.DefaultRequestHeaders["Authorization"] = "Bearer ...";

        LogMessage("Ping", "Calling");
        RestClient.Get(boardAddress+"/ping" ).Then(res =>
        {
            LogMessage("Ping", res.Text);
            PingResponse r = JsonUtility.FromJson< PingResponse>(res.Text);
            LogMessage("ping", "success:" + r.success);
        }).Catch(err => this.LogMessage("Ping", err.Message,true));
    }

    public void GetDump(System.Action<RobotState> onState) 
    {
        // We can add default request headers for all requests
        RestClient.DefaultRequestHeaders["Authorization"] = "Bearer ...";

        LogMessage("GetState", "Calling");
        RestClient.Get(boardAddress + "/dump").Then(res =>
        {
          //  LogMessage("GetState", res.Text);
            //   SumoState r = JsonUtility.FromJson<SumoState>(res.Text);
            RobotState r = JsonUtility.FromJson<RobotState>(res.Text);
            onState.Invoke(r);
           // LogMessage("GetState", "success:" + r.success);
        }).Catch(err =>
        {
            this.LogMessage("GetState", err.Message, true);
            onState.Invoke(null);
        });
    }

    //public void GetVars(System.Action<RobotState> onVars)
    //{
    //    LogMessage("GetVars", "Calling");
    //    RestClient.Get(boardAddress + "/vars").Then(res =>
    //    {
    //        //   SumoState r = JsonUtility.FromJson<SumoState>(res.Text);
    //        RobotState r = JsonUtility.FromJson<RobotState>(res.Text);
    //        onVars.Invoke(r);
    //        // LogMessage("GetState", "success:" + r.success);
    //    }).Catch(err =>
    //    {
    //        this.LogMessage("GetVars", err.Message, true);
    //        onVars.Invoke(null);
    //    });
    //}

    //public void SetMode(bool testMode)
    //{
    //    LogMessage("SetMode", "Calling");
    //    RestClient.Get(boardAddress + "/mode?testMode="+ testMode.ToString().ToLower()).Then(res =>
    //    {
    //      //  PingResponse r = JsonUtility.FromJson<PingResponse>(res.Text);
    //    }).Catch(err => this.LogMessage("SetMode", err.Message, true));
    //}

    public void SetMotor(bool isLeftMotor,int speed)
    {
        WriteVar((isLeftMotor) ? "motors.left" : "motors.right", speed);

        //LogMessage("SetMotor", "Calling");
        //RestClient.Get(boardAddress + "/motor/set?id=" + ((isLeftMotor) ?"left":"right")
        //    +"&speed="+ speed).Then(res =>
        //{
        //    //  PingResponse r = JsonUtility.FromJson<PingResponse>(res.Text);
        //}).Catch(err => this.LogMessage("SetMotor", err.Message, true));
    }
    public void SetMotorBreak(bool breakMode)
    {
        WriteVar("mode.breakMode", breakMode);

        //LogMessage("SetMotor", "Calling");
        //RestClient.Get(boardAddress + "/motor/break?breakMode=" + breakMode.ToString().ToLower()).Then(res =>
        //    {
        //        //  PingResponse r = JsonUtility.FromJson<PingResponse>(res.Text);
        //    }).Catch(err => this.LogMessage("SetMotor", err.Message, true));
    }
    public void SetGuiding(int speed,int steering)
    {
        LogMessage("SetGuiding", "Calling");
        RestClient.Get(boardAddress + "/drive?speed=" + speed+ "&steering="+ steering
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

        RestClient.Get(boardAddress + "/"+name+ (( p.Length>0) ? "?"+p:"")).Then(res =>
        {
            //  PingResponse r = JsonUtility.FromJson<PingResponse>(res.Text);
        }).Catch(err => this.LogMessage("CommandGET", err.Message, true));
    }

    public void WriteVar<T>(string name,T val )
    {
        LogMessage("WriteVar", "Calling");
        string p = "?name="+ name+"&val="+formatVal(val)+"&type="+ typeVal(val);
        RestClient.Get(boardAddress + "/write"+p).Then(res =>
        {
        }).Catch(err => this.LogMessage("WriteVar", err.Message, true));
    }
}
