using System.Linq;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

public class UISetup : UIMonoBehaviour
{
    public Robot robot;
    RobotState robotState => robot.robotState;
    RobotState nextRobotState => robot.nextRobotState;

    VisualElement root;

    bool ipMode = false;

    public GameObject controlGO;

    // Start is called before the first frame update
    void OnEnable()
    {
        robot.OnStateChanged = () =>
          {
              UpdateStateBar();
          };
        robot.OnValueChanged = () =>
        {
            UpdateDump();
        };


        var doc = GetComponent<UIDocument>();
        root = doc.rootVisualElement;

        root.Q<Label>("lblName").text = "Disconnected";
        root.Q<Label>("lblDump").text = "";

        root.Q<Label>("lblAddress").text = robot.client.boardAddress;

        root.Q<Button>("btnIP").clicked += () =>
        {
            ipMode = !ipMode;
            if (ipMode)
            {
                root.Q<TextField>("txtAddress").value = robot.client.boardAddress;
            }
            else
            {
                robot.client.boardAddress = root.Q<TextField>("txtAddress").value;
                root.Q<Label>("lblAddress").text = robot.client.boardAddress;

                PlayerPrefs.SetString("boardAddress", robot.client.boardAddress);
                PlayerPrefs.Save();

            }
            UpdateStateBar();
        };

        // ================
        // actions

        root.Q<Button>("btnResetMPU").clicked += () =>
        {
            robot.client.CommandGET("resetMPU");
        };

        root.Q<Button>("btnDrive").clicked += () =>
        {
            StartCoroutine(Co_Action(() =>
            {
                controlGO.SetActive(true);
                gameObject.SetActive(false);
            }));
        };

        //
        UpdateStateBar();
    }

    void UpdateStateBar()
    {
        robot.startRobotState.VisualizeCommand(root.Q<Button>("btnResetMPU"),"resetMPU");

        root.Q<Label>("lblName").text = robot.startRobotState.name;

        if (ipMode)
        {
            root.Q<Label>("lblAddress").style.display = DisplayStyle.None;
            root.Q<Label>("lblAddress").style.visibility = Visibility.Hidden;
            root.Q<TextField>("txtAddress").style.display = DisplayStyle.Flex;
            root.Q<TextField>("txtAddress").style.visibility = Visibility.Visible;
            root.Q<Button>("btnIP").text = "Save";
        }
        else
        {
            root.Q<Label>("lblAddress").style.display = DisplayStyle.Flex;
            root.Q<Label>("lblAddress").style.visibility = Visibility.Visible;
            root.Q<TextField>("txtAddress").style.display = DisplayStyle.None;
            root.Q<TextField>("txtAddress").style.visibility = Visibility.Hidden;
            root.Q<Button>("btnIP").text = "Set IP";
        }
    }
    void UpdateDump()
    {
        root.Q<Label>("lblDump").text = robot.robotState.ToProps();
    }
}
