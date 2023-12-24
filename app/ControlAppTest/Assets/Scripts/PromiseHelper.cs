using Proyecto26;
using RSG;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
#if UNITY_EDITOR
using UnityEditor;
#endif
using UnityEngine;


static class PromiseHelper
{
    public static void Promisify1<T>(this Promise<T> promise, RequestException error, T response)
    {
        if (error != null) { promise.Reject(error); } else { promise.Resolve(response); }
    }
}
