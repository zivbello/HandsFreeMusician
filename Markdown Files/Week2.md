# Week 2
Group 8 Members: Ziven Bello, Kimberly Talisse, Leon Chen
___

## Mon, Mar 30 (12:30 - 3:30)
- Redesigned pen holder to better fit the stylus and have a longer arm
- Tried to install the computer vision algorithms we found into a virtual environment but they both were too large
- The second algorithm was built for ASL interpreting rather than general hand gestures, so we will not use that one
- Researching more algorithms to work with
- found this hand gesture database: https://www.kaggle.com/datasets/gti-upm/leapgestrecog/data
- Found deep-learning gesture recognition model: https://github.com/guillaumephd/deep_learning_hand_gesture_recognition?tab=readme-ov-file
- Considering running models on our own laptops to avoid issues with installing mediapipe on R-Pi
- Also found this model: https://github.com/fabiopk/RT_GestureRecognition
- Modeled several potential gears for the chain
- Got mediapipe working on a laptop, will need to train for more gestures or find another model that uses mediapipe
- Will work on refining gesture recognition and implement communication with Arduino
- Will figure out (if possible) how to get code running on R-Pi
- Got a thumb drive to help with prototyping 3d printed parts

## Mon, Apr 1 (12:30 - 3:30)
- Changed the hand gesture to: Thumbs Up → BPM +10, Thumbs Down → BPM -10, Fist → Stop, Peace → Resume
- Used Mediapipe on a personal computer and got the gesture and camera working
- Copied gesture code and model file to Raspberry Pi, gesture recognition running live on Raspberry Pi camera now
- Printed out prototype parts in preparation for arrival of ordered parts
- Programmed the Arduino for testing gesture control
- Got the Pi to communicate with the Arduino
- Attached servo to Arduino to test stylus holder
- Need new servo as the lab ones are far too weak and overheat easily
- Ordered new servo
