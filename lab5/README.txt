Additional Features:

1. User Registration

Overview
- Our application now supports a /register command on the client side where the users can register as new users. 

Implementation
- Users and their passwords are stored on the users.txt file. If the client tries to register a user that already exists it does not allow them to
  as it searches through the text file for the user. 

2. Private Messaging

Overview
- Users are able to send private messages to one another using /pm <user> <message>, where the <user> is the recipient you want to contact, and the <message>
  is the message that you want to send. You are only able to send messages to users that are connected, and only if you are logged in yourself. The message is
  sent regardless of if the user is currently in a session or just logged in. 

Implementation
- On the client side, we send a package to the server with the username and the message contatinated as "user:message".
- On the server side, we recieve that package from the client side, splits the package with respect to the user and message. It iterates through a linked list
  of connected clients, checking if the user is currently connected. If so, send the message, if not, then return user not found. 