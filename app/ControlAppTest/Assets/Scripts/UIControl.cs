using System.Linq;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

public class UIControl : UIMonoBehaviour
{
    public Robot robot;
    Vector2 nativeSize = new Vector2(640, 480);
    public GameObject setupGO;
    RobotState robotState => robot.robotState;
    RobotState nextRobotState => robot.nextRobotState;
    // RobotState nextRobotState = null;

    Label lblSensors;
    Label lblState;
    VisualElement root;

  //  bool ipMode = false;

    GUIStyle backStyle;
    GUIStyle lineStyle;
    GUIStyle midStyle;

    private Texture2D MakeTex(int width, int height, Color col)
    {
        Color[] pix = new Color[width * height];
        for (int i = 0; i < pix.Length; ++i)
        {
            pix[i] = col;
        }
        Texture2D result = new Texture2D(width, height);
        result.SetPixels(pix);
        result.Apply();
        return result;
    }


    // Start is called before the first frame update
    void OnEnable()
    {
        robot.OnStateChanged = () =>
        {
            UpdateStateBar();
        };
        robot.OnValueChanged = () =>
         {
             UpdateGUI();
         };
     //   StartCoroutine(UpdateState());
       // StartCoroutine(UpdateCommands());

        var doc = GameObject.FindAnyObjectByType<UIDocument>();
        root = doc.rootVisualElement;

        lblSensors = root.Q<Label>("lblSensors");

        //lblSensors.text = "..";

        root.Q<Button>("btnUpdateState").clicked += () =>
         {
             robot.nextRobotState = null;
             robot.GetRunningState();
             UpdateGUI();
         };
        root.Q<Button>("btnMode").clicked += () =>
        {
            if (robotState.mode.workMode == WorkMode.DriveNormal)
                robotState.mode.workMode = WorkMode.MotorTest;
            else
                robotState.mode.workMode = WorkMode.DriveNormal;

            robot.client.WriteVar("mode.workMode", (int)robotState.mode.workMode);

            UpdateGUI();
        };
        root.Q<Button>("btnBreak").clicked += () =>
        {
            robot.client.SetMotorBreak(!robotState.mode.breakMode);
            UpdateGUI();
        };

        root.Q<SliderInt>("sldLeft").RegisterValueChangedCallback( (evt) =>
        {
            nextRobotState.motors.left = evt.newValue;
        }); 
        root.Q<Button>("btnLefStop").clicked += () =>
        {
            root.Q<SliderInt>("sldLeft").value = 0;
            UpdateGUI();
        };

        root.Q<SliderInt>("sldRight").RegisterValueChangedCallback((evt) =>
        {
            nextRobotState.motors.right = evt.newValue;
        });
        root.Q<Button>("btnRightStop").clicked += () =>
        {
            root.Q<SliderInt>("sldRight").value = 0;
            UpdateGUI();
        };
        root.Q<Button>("btnSetup").clicked += () =>
        {
            StartCoroutine(Co_Action(() =>
            {
                setupGO.SetActive(true);
                gameObject.SetActive(false);
            }));
        };

    
        // =====
        //root.Q<SliderInt>("sldSpeed").RegisterValueChangedCallback((evt) =>
        //{
        //    nextRobotState.speed = evt.newValue;
        //});
        //root.Q<SliderInt>("sldSteering").RegisterValueChangedCallback((evt) =>
        //{
        //    nextRobotState.steering = evt.newValue;
        //});
        root.Q<Button>("btnStopSpeed").clicked += () =>
        {
            // root.Q<SliderInt>("sldSpeed").value = 0;
            nextRobotState.driver.speed = 0;
            UpdateGUI();
        };
        root.Q<Button>("btnStopSteering").clicked += () =>
        {
            //   root.Q<SliderInt>("sldSteering").value = 0;
            nextRobotState.driver.steering = 0;
            UpdateGUI();
        };

        UpdateStateBar();
        UpdateGUI();
    }

    void UpdateStateBar()
    {
        root.Q<Label>("lblName").text = robot.startRobotState.name;
        root.Q<Label>("lblAddress").text = robot.client.boardAddress;

        UpdateGUI();
    }

    private void UpdateGUI()
    {
        root.Q<VisualElement>("vsTestMode").style.display = DisplayStyle.None;
        root.Q<VisualElement>("vsTestMode").style.visibility = Visibility.Hidden;
        root.Q<VisualElement>("vsGuiding").style.visibility = Visibility.Hidden;
        if (robotState != null && nextRobotState!=null)
        {
           
        
            root.Q<Button>("btnMode").text = (robotState.mode.workMode == WorkMode.MotorTest ? "Test Mode" : "Guiding");
            root.Q<Button>("btnBreak").text = (robotState.mode.breakMode ? "START" : "BREAK");

            root.Q<VisualElement>("vsTestMode").style.visibility = robotState.mode.workMode == WorkMode.MotorTest ? Visibility.Visible : Visibility.Hidden;
            root.Q<VisualElement>("vsGuiding").style.visibility = robotState.mode.workMode !=  WorkMode.MotorTest ? Visibility.Visible : Visibility.Hidden;

            if (robotState.mode.workMode == WorkMode.MotorTest)
            {
                root.Q<VisualElement>("vsTestMode").style.display = DisplayStyle.Flex;
                root.Q<Label>("lblLeftMotor").text = "" + nextRobotState.motors.left;
                root.Q<Label>("lblRightMotor").text = "" + nextRobotState.motors.right;
            }
            if (robotState.mode.workMode != WorkMode.MotorTest)
            {
                //root.Q<Label>("lblGuideState").text = "Motors     Left:" + robotState.left + " Right:" + robotState.right+
                //  "\nGuiding  Speed:" + robotState.speed + " Steering:" + robotState.steering;

             //   root.Q<SliderInt>("sldSpeed").label = "Speed: " + nextRobotState.speed;
              //  root.Q<SliderInt>("sldSteering").label = "Steering: " + nextRobotState.steering;

            }
          
        }

    }

    Rect GetRect(bool isLeft,bool outBorder)
    {
        Rect rect;
        if (outBorder)
        {
            if (isLeft)
                rect = getScreenSpaceRect(root.Q<VisualElement>("vsSpeed").parent);
            else
                rect = getScreenSpaceRect(root.Q<VisualElement>("vsSteering").parent);
        }
        else
        {
            if (isLeft)
                rect = getScreenSpaceRect(root.Q<VisualElement>("vsSpeed"));
            else
                rect = getScreenSpaceRect(root.Q<VisualElement>("vsSteering"));
        }

        //rect.position = rect.position + new Vector2(border, border);
        //rect.size = rect.size  - new Vector2(border*2, border*2);
        return rect;
    }
    float midSide => GetRect(true, true).xMax;
    bool isLeftSide(float x) => x < midSide;

    List<Vector2> GetCurrentInputs()
    {
        List<Vector2> list = new List<Vector2>();
        if (Application.platform == RuntimePlatform.Android)
        {
            for (int i = 0; i < Input.touchCount; i++)
            {
                Touch touch = Input.GetTouch(i);
                var find = list.FirstOrDefault( X=> isLeftSide(X.x) == isLeftSide(touch.position.x));
                if (find== default(Vector2))
                    list.Add(touch.position);
            }
        }
        else
        {
            if (Input.GetMouseButton(0))
            {
                list.Add(Input.mousePosition);
            }
        }
        return list;
    }

    private void Update()
    {
        if (robotState != null)
        {
            string txt = "";

            if (robotState.distance.enable)
                txt += "Dist:" + robotState.distance.valueCM;
            if (robotState.line.enable)
                txt += "Line:" + robotState.line.value;
            if (robotState.MPU.enable)
                txt += "MPU Ang:" + robotState.MPU.sensor_angle;

            txt += string.Format("Left:{0} Right:{1} Speed:{2} Steering:{3}",
                  robotState.motors.left,  robotState.motors.right, robotState.driver.speed, robotState.driver.steering);

            lblSensors.text = txt;
        }
        else
            lblSensors.text = "";

        // -------------

        if (nextRobotState != null && robotState.mode.workMode != WorkMode.MotorTest)
        {
            if (Application.platform == RuntimePlatform.Android)
            {
                nextRobotState.driver.speed = 0;
                nextRobotState.driver.steering = 0;
            }

            List<Vector2> inputs = GetCurrentInputs();
            for(int i=0;i< inputs.Count;i++)
            {
                Vector2 pos = inputs[i];

                if (pos.x < midSide)
                {
                    var rect = GetRect(true,false);

                    float speed = Mathf.Clamp01(
                        (float)((Screen.height - pos.y) - rect.yMin) / rect.height);
                    speed = 100f * (-1 + (1f - speed) * 2);

                    Debug.Log("speed :" + speed);
                    nextRobotState.driver.speed = (int)speed;
                }
                else
                {
                    var rect = GetRect(false, false);

                    float steering = Mathf.Clamp01(
                      (float)( pos.x - rect.xMin) / rect.width);
                    steering = 100f * (-1 + (1f - steering) * 2);

                    Debug.Log("steering :" + steering);
                    nextRobotState.driver.steering = (int)steering;

                }
            }
        }

    }

    private void OnGUI()
    {  
        if (backStyle == null)
        {
            backStyle = new GUIStyle(GUI.skin.box);
            backStyle.normal.background = MakeTex(2, 2, new Color(0.5f, 0.5f, 0.5f, 0.5f));
            lineStyle = new GUIStyle(GUI.skin.box);
            lineStyle.normal.background = MakeTex(2, 2, new Color(0f, 1f, 0f));
            midStyle = new GUIStyle(GUI.skin.box);
            midStyle.normal.background = MakeTex(1, 1, new Color(1f, 1f, 1f));
        }

        if (nextRobotState != null && robotState.mode.workMode!=  WorkMode.MotorTest)
        {
          
            // speed
            var rectSpeed =GetRect(true,true);
            GUI.Box(rectSpeed, "");
            rectSpeed = GetRect(true, false);

            float factor = rectSpeed.height - (0.5f+ (float)nextRobotState.driver.speed / 200) *  (rectSpeed.height);
            var rectLine = new Rect(rectSpeed.xMin, rectSpeed.yMin +  factor , rectSpeed.width , 2);

            GUI.Box(rectSpeed, "", backStyle);
            GUI.Box(new Rect(new Vector2(rectSpeed.xMin, rectSpeed.center.y),
                new Vector2( rectSpeed.width, 1)), "", midStyle);
            GUI.Box(rectLine, "", lineStyle);
            

            // steering
            var rectSteering = GetRect(false,true);
            GUI.Box(rectSteering, "");
            rectSteering = GetRect(false,false);

            factor = rectSteering.width - (0.5f + (float)nextRobotState.driver.steering / 200) * (rectSteering.width);
            rectLine = new Rect(rectSteering.xMin+ factor, rectSteering.yMin, 2, rectSteering.height);

            GUI.Box(rectSteering, "", backStyle);
            GUI.Box(new Rect(new Vector2(rectSteering.center.x, rectSteering.yMin),
             new Vector2(1, rectSteering.height)), "", midStyle);
            GUI.Box(rectLine, "", lineStyle);

        }
    }

    public Rect getScreenSpaceRect(VisualElement visual)
    {
        IPanel panel = visual.panel;
        Vector2 screenBLPanel = RuntimePanelUtils.ScreenToPanel(panel, new Vector2(0, 0));
        Vector2 screenTRPanel = RuntimePanelUtils.ScreenToPanel(panel, new Vector2(Screen.width, Screen.height));
        Rect panelRect = visual.worldBound;
        Vector2 scale = new Vector2(Screen.width, Screen.height) / screenTRPanel;

        Vector2 min = visual.worldBound.min * scale;
        Vector2 max = visual.worldBound.max * scale;

        return new Rect(min , max - min);
    }

    //private void OnGUI1()
    //{
        
    //    Vector3 scale = new Vector3(Screen.width / nativeSize.x, Screen.height / nativeSize.y, 1.0f);
    //    GUI.matrix = Matrix4x4.TRS(new Vector3(0, 0, 0), Quaternion.identity, scale);

    //    GUILayout.BeginVertical();
    //    GUILayout.Label("SUMO Robot");

    //    if (GUILayout.Button("Update State"))
    //    {
    //        robot.GetState();
    //    }
    //    if (robotState != null)
    //    {
    //        GUILayout.Label("Sensors     Dist:" + robotState.distance.valueCM + " Line:" + robotState.line.value);
    //        GUILayout.Label("Motors     Left:" + robotState.motors.left + " Right:" + robotState.motors.right);
    //        GUILayout.Label("Guiding     Speed:" + robotState.driver.speed + " Steering:" + robotState.driver.steering);
    //        //  GUILayout.Label("State: " + robotState.ToString());

    //        if (GUILayout.Button("Mode " + (robotState.mode.testMode ? "Test Mode" : "Guiding")))
    //        {
    //            robot.client.SetMode(!robotState.mode.testMode);
    //            robotState.mode.testMode = !robotState.mode.testMode;
    //        }
    //        if (robotState.mode.breakMode)
    //        {
    //            if (GUILayout.Button("START", GUILayout.Height(50)))
    //            {
    //                robot.client.SetMotorBreak(false);
    //            }
    //        }
    //        else
    //        {
    //            if (GUILayout.Button("STOP", GUILayout.Height(50)))
    //            {
    //                robot.client.SetMotorBreak(true);
    //            }
    //        }
    //        if (!robotState.mode.breakMode && robotState.mode.testMode)
    //        {
    //            GUILayout.BeginHorizontal();
    //            GUILayout.Label("Motor Left: ");
    //            nextRobotState.motors.left = (int)GUILayout.HorizontalSlider(nextRobotState.motors.left, -100, 100, GUILayout.Width(200));
    //            GUILayout.Label("" + nextRobotState.motors.left);
    //            GUILayout.EndHorizontal();

    //            GUILayout.BeginHorizontal();
    //            GUILayout.Label("Motor Right: ");
    //            nextRobotState.motors.right = (int)GUILayout.HorizontalSlider(nextRobotState.motors.right, -100, 100, GUILayout.Width(200));
    //            GUILayout.Label("" + nextRobotState.motors.right);
    //            GUILayout.EndHorizontal();
    //        }
    //    }
    //    else
    //    {
    //        GUILayout.Label("DISCONNECTED");
    //    }

    //    //if (GUILayout.Button("PING"))
    //    //{
    //    //    client.Ping();
    //    //}
    //    //GUILayout.Label("Activity: " + client.currentActivity);
    //    GUILayout.EndVertical();
    //}
}
