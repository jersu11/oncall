# On Call #
We needed a way to provide a single telephone number that can be used to connect with a team of people, 24 hours a day. The makeup of the team needed to be able to change at any time, as team members rotate into the role of 'team leader' or become unavailable because of vacation or other commitments. 

'On Call', a Drupal 7 module that leverages the power of the [Twilio](http://www.twilio.com) cloud communication platform, was born from this need. It lets a person contact an 'on call' team from a single telephone number. The person contacting the team would typically be requesting help or reporting a problem, and need some type of immediate assistance. The caller is given an option to leave a voice mail or to immediatly connect with the 'on call' team leader's phone number. If the voice mail option is chosen, an SMS message is sent to all members of the 'on call' team, which includes a link to listen to the voice recording.

The team are able to communicate with each other via SMS to coordinate problem solving. Team members are able to leave status messages that can be heard by any person calling the number. 

## Workflow ##
A typical workflow might look like this:

* A client notices that his website is down. He calls the 'on call' telephone number. He chooses option 1, to leave a voice mail for the team

* The entire 'on call' team is sent an SMS message that includes a link to listen to the message. 

* Team member 'Joe' replies to the SMS to say that he'll look into the issue. Everyone on the 'on call' team gets Joe's SMS.

* Team member 'Joe' calls the 'on call' line to update the issue status. He says "It looks like the webserver is stuck". If anyone else calls the 'on call' line, they'll hear this status and know that the issue is being worked on. 

* Team member 'Joe' sends an SMS to the team to say 'Jack, can you restart the webserver'

* The client's website starts working again. He's happy. He calls the 'on call' line again. This time he chooses option 2 to connect directly with the team leader. The client wants to tell the team leader in person how happy he is with the speed of the problem resolution.

## Install ##
This module requires the Twilio API for PHP module which can be found here: [https://github.com/jersu11/twiliophp](https://github.com/jersu11/twiliophp)

## Changelog ##
This module now drops its dependence on the Drupal User interface and manages the 'on call' team from within the module's administrative interface (on next checkin)

