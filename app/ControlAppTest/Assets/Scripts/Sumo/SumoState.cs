using Proyecto26;
using System.Collections;
using System.Collections.Generic;
#if UNITY_EDITOR
using UnityEditor;
#endif
using UnityEngine;

//[System.Serializable]
//public class SumoState : RobotState
//{
//    public float dist_cm;
//    public bool lineON;

//    public override string ToString()
//    {
//        return UnityEngine.JsonUtility.ToJson(this, true);
//    }
//}

[System.Serializable]
public class PingResponse
{
    public bool success;
    public override string ToString()
    {
        return UnityEngine.JsonUtility.ToJson(this, true);
    }
}