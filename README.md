# IDEAHacks21
Am I Busy? Collaborative project with Grace Ma for IDEAHacks21 

This project was the result of 2 days during a remote Hackathon, with the help of various volunteers from UCLA.
It utilized hardware in the form of an ESP32, OLED Display, push button, and IR emitter pairing. 
HTML, Javascript, and CSS, created a website intended for a aesthetic UI and called upon the necessary API required to communicate with the database. 

Its purpose is to avoid situations where someone enters the users room during the wrong time such as doing homework or in a work call. There are two aspects to the system, one for the user, and one for those living with the user. The user has access to a website that acts as a daily planner, and whenever they add events to it, they are uploaded to the database via the API-spreadsheets. The physical system, which connects via wifi, can access the database through an API call and determines whether or not the user is busy. If those living with the user are curious if they're free or not, they can simply go to the prototype instead of potentially disturbing them. There are corresponding messages with each situation, and the RGB led with turn red or green accordingly.

Presentation Slides: https://docs.google.com/presentation/d/1mqqkInF__Lfuej-5Vy9nJ_eKrnEyseAHpMVVbUxwlf0/edit?usp=sharing
