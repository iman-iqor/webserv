.# webserv
imane added something
int nfds = epoll_wait(epoll_fd, events, 10, -1);
🧠 What this line REALLY does

👉 It means:

“Stop here and wait until some socket becomes active”

🔥 Break it down simply
epoll_wait(...) = waiting for activity

Your program literally pauses here:

😴 waiting...

Until:

a client sends data
a new connection arrives
📦 The parameters
epoll_wait(epoll_fd, events, 10, -1);
1. epoll_fd

👉 the thing watching all your sockets

2. events

👉 an array where results will be stored

After epoll_wait, it will contain:

“these sockets are ready”

3. 10

👉 maximum number of events to return

Meaning:

“give me at most 10 ready sockets”

4. -1

👉 wait forever

Other options:

0 → don’t wait (instant return)
1000 → wait 1 second
⚡ What does it return?
int nfds

👉 number of sockets that are ready

🎯 Example

Let’s say you are watching 3 sockets:

socket1
socket2
socket3

Now:

only socket2 has data

👉 then:

nfds = 1

And:

events[0].data.fd = socket2
🧠 After epoll_wait

You loop:

for (int i = 0; i < nfds; i++) {

👉 only on ready sockets

🧩 Super simple analogy

Imagine:

you are waiting for messages 📩
instead of checking every chat manually

👉 your phone wakes you up only when:

“you got a message from THIS person”

That’s epoll_wait

🔥 Equivalent in poll (what YOU will use)

This:

epoll_wait(...)

is basically:

poll(_pollfds.data(), _pollfds.size(), -1);

Same idea:
👉 wait until something happens

🎯 One sentence to remember

👉 epoll_wait = “sleep until a socket is ready, then tell me which ones”