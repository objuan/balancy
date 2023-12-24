using System.Collections;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using UnityEngine.UIElements;
#if UNITY_EDITOR
using UnityEditor;
#endif
using UnityEngine;


public class Robot : MonoBehaviour
{
    // public WemosRestClient client;
    public WemosTCPClient client;

    public RobotState startRobotState;

    public bool IsOnline => startRobotState != null;

    [HideInInspector]
    public RobotState robotState;
    [HideInInspector]
    public  RobotState nextRobotState;

    public System.Action OnStateChanged;
    public System.Action OnValueChanged;

    bool firstTime = true;

    private void Awake()
    {
        client.boardAddress = PlayerPrefs.GetString("boardAddress", client.boardAddress);
       
    }

    void Start()
    {
        client.Start();

        StartCoroutine(UpdateState());
        StartCoroutine(UpdateCommands());
    }
    private void Update()
    {
        client.Update();
    }


    IEnumerator UpdateState()
    {
        while (true)
        {
            client.UpdateSlow();

            //if (!sumoState.testMode)
            if (firstTime)
            {
                firstTime = false;
                GetFirstState();
            }
            else
                GetRunningState();

            yield return new WaitForSeconds(1f);
        }
    }
    IEnumerator UpdateCommands()
    {
        while (true)
        {
            if (nextRobotState != null)
            {
                if (robotState.mode.workMode == WorkMode.MotorTest)
                {
                    if (nextRobotState.motors.left != robotState.motors.left)
                    {
                        robotState.motors.left = nextRobotState.motors.left;
                        client.SetMotor(true, robotState.motors.left);
                    }
                    if (nextRobotState.motors.right != robotState.motors.right)
                    {
                        robotState.motors.right = nextRobotState.motors.right;
                        client.SetMotor(false, robotState.motors.right);
                    }
                }
                else
                {
                    if (nextRobotState.driver.steering != robotState.driver.steering
                        || nextRobotState.driver.speed != robotState.driver.speed)
                    {
                        robotState.driver.steering = nextRobotState.driver.steering;
                        robotState.driver.speed = nextRobotState.driver.speed;

                        client.SetGuiding(robotState.driver.speed, robotState.driver.steering);
                    }
                }
            }
            yield return new WaitForSeconds(.1f);
        }
    }

    public void GetFirstState()
    {
        client.GetDump((S) =>
        {
            startRobotState = S;
            OnStateChanged.Invoke();
        });
    }

    public void GetRunningState()
    {
        client.GetDump((S) =>
        {
            robotState = S;
            //  Debug.Log(S.left);
            if (robotState != null && nextRobotState == null)
            {
                nextRobotState = new RobotState()
                {
                    driver = new Driver() { speed = S.driver.speed, steering = S.driver.steering },
                    motors = new Motors()
                    {
                        left = S.motors.left,
                        right = S.motors.right
                    },
                    mode = new Mode() { workMode = S.mode.workMode }
                };
                // init
                //root.Q<SliderInt>("sldLeft").value = S.left;
                //root.Q<SliderInt>("sldRight").value = S.right;
                //root.Q<SliderInt>("sldSpeed").value = S.speed;
                //root.Q<SliderInt>("sldSteering").value = S.steering;
            }

            if (S == null) nextRobotState = null;
            OnValueChanged?.Invoke();
        });
    }

  
}

[System.Serializable]
public class MPU
{
    public bool enable;
    public int sensor_angle;
    public int angle_adjusted;
}

[System.Serializable]
public class SensorDistance
{
    public bool enable;
    public float valueCM;
}

[System.Serializable]
public class SensorLine
{
    public bool enable;
    public bool value;
}

[System.Serializable]
public class Driver
{
    public int steering;
    public int speed;
}

[System.Serializable]
public class Motors
{
    public int left;
    public int right;
}

public enum WorkMode : int
{
    DriveNormal=0,
    DrivePro=1,
    MotorTest = 2
}

[System.Serializable]
public class Mode
{
    public bool breakMode;
    public WorkMode workMode;
}

[System.Serializable]
public class RobotState
{
    public string name;
    public string[] cmds;
    public Mode mode;
    public MPU MPU;
    public SensorDistance distance;
    public SensorLine line;
    public Motors motors;
    public Driver driver;

    public override string ToString()
    {
        return UnityEngine.JsonUtility.ToJson(this, true);
    }

    public bool HasCommand(string name)
    {
        return cmds.Contains(name);
    }
    public void VisualizeCommand(VisualElement vs, string name)
    {
        if (HasCommand(name))
        {
            vs.style.display = DisplayStyle.Flex;
            vs.style.visibility = Visibility.Visible;
        }
        else
        {
            vs.style.display = DisplayStyle.None;
            vs.style.visibility = Visibility.Hidden;
        }
    }
    void Add<T>(StringBuilder str, string name, T value) {
        str.Append(name);
        str.Append(":");
        str.Append(value.ToString());
        str.Append('\n');
    }
    public string ToProps()
    {
        StringBuilder str = new StringBuilder();

        Add(str, "name",name);
        if (cmds != null)
        {
            foreach (var cmd in cmds)
            {
                Add(str, "cmds." + cmd, "action");
            }
        }
        Add(str, "mode.breakMode", mode.breakMode);
        Add(str, "mode.workMode", (int)mode.workMode);
        if (MPU.enable)
        {
            Add(str, "MPU.sensor_angle", MPU.sensor_angle);
            Add(str, "MPU.angle_adjusted", MPU.angle_adjusted);
        }
        if (line.enable)
        {
            Add(str, "line.value", line.value);
        }
        if (distance.enable)
        {
            Add(str, "distance.valueCM", distance.valueCM);
        }
        Add(str, "motors.left", motors.left);
        Add(str, "motors.right", motors.right);
        Add(str, "driver.steering", driver.steering);
        Add(str, "driver.speed", driver.speed);

        return str.ToString();
    }
}

