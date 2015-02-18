#pragma once

#include <unistd.h>
#include "Shared/msg_structs.h"

int msg_recv(struct msg_header *msg);
int msg_send(struct msg_header *msg);
