using System;
using System.Collections;

using UnityEngine;
using UnityEngine.UIElements;

public class UIMonoBehaviour: MonoBehaviour
{
    public IEnumerator Co_Action(Action action)
    {
        action();
        yield return action;
    }

}
