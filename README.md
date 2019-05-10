# GameLift Example in UE4

## About
This repository is an example project that implements the basic features of AWS GameLift in Unreal Engine.

I would highly suggest watching my tutorials on youtube before jumping straight in so that you have a better understanding of GameLift.

- [How to host a Unreal Engine Dedicated Server on Amazon Gamelift (PART 1/2)](https://youtu.be/Iq2LpwXogTw)
- [How to host a Unreal Engine Dedicated Server on Amazon Gamelift (PART 2/2)](https://youtu.be/2I8JDeMGkgc)

## What is GameLift

GameLift is a service fully managed by Amazon that is used for deploying multiplayer session-based dedicated servers to the cloud.

## How to build and run the project

### Step 1: Add the necessary dll and lib files for the plugins

This project relied on the amazon gamelift client sdk, which can be found here. You can either download the whole sdk or preferably download just the dll and lib from the ThirdParty folder found here. Now go to the GameLiftTutorial project in File Explorer, and navigate to the folder's Plugins -> GameLiftClientSDK, and create a new folder here. Title this folder, "ThirdParty". Open this folder, and create a new folder here. Title this folder "GameLiftClientSDK". Open this folder, and a create another new folder. Title this folder "Win64". Open this folder, and copy and paste the dll and lib files downloaded from earlier into this folder.

This project also used the amazon gamelift server sdk. However, you only need these files if you plan on deploying this project to a gamelift server, either remotely or locally, since you will need to build server files. Note that to build server files, you will need the source version of Unreal Engine. Instructions on getting the dll and lib files for the server sdk can be found [here](https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-engines-setup-unreal.html). Once you have those files, open the Fortnite Clone project in File Explorer and navigate to the folders Plugins -> GameLiftServerSDK -> ThirdParty -> GameLiftServerSDK -> Win64, and copy and paste the files there.

**Note** that I am currently using Unreal version 4.22.0 and Visual Studio 2019, but to build these GameLiftServerSDK files you need to use Visual Studio 2015 or 2017 build tools, specifically for running the cmake and msbuild commands. However, I've noticed that having multiple version of Visual Studio with the 2019 version made Unreal Engine default to using an older version so I only installed Visual Studio 2015/2017 for this step, and recommend removing it right after if it gives you problems later down the road. Using an older version of Visual Studio may involve having to adding a VCTargetsPath system environment variable, which I go over [here](https://answers.unrealengine.com/questions/884106/tutorial-on-integrating-unreal-with-amazon-gamelif.html).

### Step 2: Packaging and preparing for uploading to GameLift

Open up the uproject file, and you may be prompted to build the project and plugins. After that's completed, go to the "Maps" folder in the Content Browser and open up the file called "Level_Entry". Up in the toolbar, click the carat by where it says "Blueprints" and select "Open Level Blueprint". In the "Create Game Lift Object" node, add your amazon access and secret key credentials. And if you have a game session id already, add that to the "Create Player Session" node. If not, then add this later and repackage the project. After that's completed, open Visual Studio and compile the project only in both the "Development Editor" and "Development Server" configurations. Then, go back to Unreal Engine and package the project for either Windows or Linux by going to File -> Package Project -> [Select Platform].

Once the project is packaged, you should see a new folder appear in your project in File Explorer called "WindowsNoEditor" (assuming you built for Windows). Open this folder, and add a new file to it called "install.bat". Edit this batch file to include these two lines (assuming you've packaged the project for Windows 64-bit),

```
vc_redist.x64.exe /q
Engine\Extras\Redist\en-us\UE4PrereqSetup_x64.exe /q
```
Also, add the visual c++ redistributable, which can be downloaded [here](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads).

Then, go back the main project's folder in File Explorer, and copy the GameLiftServerSDK folder inside the Plugins folder. Back to the main project's top directory, head over to WindowsNoEditor -> GameLiftTutorial -> Plugins and paste the folder there. Then, go back to the main project's folder in File Explorer again, and go to Binaries -> Win64 and copy all the files starting with "GameLiftTutorialServer" (should be five files). Back to the main project's top directory, head over to WindowsNoEditor -> GameLiftTutorial -> Binaries -> Win64 and paste the files there. Your project should now be ready for uploading to GameLift! Steps for how to do that can be found (here)[https://docs.aws.amazon.com/gamelift/latest/developerguide/gamelift-build-cli-uploading.html].

For any issues related to building/packaging, please refer to my post on the Unreal Q&A forums first where I cover a lot of potential causes for errors: https://answers.unrealengine.com/questions/884106/tutorial-on-integrating-unreal-with-amazon-gamelif.html
