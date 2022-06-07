# Overview

For this design exercise, I implemented seven different state machines as described below:

The first state machine (modeState) governs whether the clock is in hour:minute mode or hour:minute:second mode. The clock starts out in modeState=0 (hour:minute mode), and when Button 1 is pressed, modeState changes to 1 (hour:minute:second mode). The second state machine (buttonState) is responsible for determining whether the first button has been pressed. In conjunction with current button reading, the buttonState variable stores whether the button was previously pressed or unpressed (to record a full press action).

Next, I have a state machine (mode2State) to govern whether the system is in ALWAYS_ON mode or CHECK_ON mode (checking for idle time). The state changes based on whether the user presses button 2, and these presses are kept track of by the button2State variable. When the mode2State machine is in CHECK_ON mode, there is another state machine (screenState) to regulate whether the screen is currently on or off. This state machine has two states - ACTIVATED or DEACTIVATED.

Finally, two more state machines are implemented to improve the user experience. The colonState machine regulates whether the colon is displayed or not displayed on the screen when in hour:minute mode. To add AM or PM to the time reading, a timeScale state machine is added with AM and PM modes.

I also implemented four timers: a one second timer (to increment seconds since the GET request only occurs once per minute), a half second timer (to flash the colon in hour:minute mode), a fifteen second timer (to check for idle time when on CHECK_ON mode), and a one minute timer (to do the GET requests).

[demonstration video](https://www.youtube.com/watch?v=GqoFyI8VGLg)

Here is an example of a how I am using the millis() function to regulate how often the GET request occurs:

```cpp
if (millis() - minutetimer > 60000) {
    updateTime();
    minutetimer = millis();
}
```

Since the GET request is only occurring once per minute, I used the following code to increment seconds:

```cpp
if (millis() - secondtimer >=1000) {
    secs+=1;
    if (secs>59) {
        secs = 0;
        mins +=1;
    }
    secondtimer = millis();
}
```

# Summary

Overall, all of the given specifications have been implemented in the system I designed. This includes using the LCD to render current time, having two display modes (button controlled), having a flashing colon in hour:minute mode, doing a HTTP GET request no more than once per minute, switching away from always on mode with a second button, and checking for motion with the IMU. These components come together to create a working smart clock that is both functional and easy-to-use.