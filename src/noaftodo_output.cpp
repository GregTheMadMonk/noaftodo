#include "noaftodo_output.h"

#include <iostream>
#include <libnotify/notify.h>

using namespace std;

void log(const string& message, const char& prefix)
{
	cout << "[" << prefix << "] " << message << endl;
}

void notify(const string& title, const string& description, const int& priority)
{
	NotifyNotification* n = notify_notification_new(title.c_str(), description.c_str(), 0);
	notify_notification_set_timeout(n, 5000);

	NotifyUrgency u = NOTIFY_URGENCY_LOW;
	if (priority == NP_MID) u = NOTIFY_URGENCY_NORMAL;
	if (priority == NP_HIGH) u = NOTIFY_URGENCY_CRITICAL;

	notify_notification_set_urgency(n, u);

	notify_notification_show(n, 0);
}
