#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include "aminotis_wire.h"

void an_use_cursor(struct ANRequest *request, const struct ANStatus *status)
{
    request->epoch = status->epoch;
    request->epoch_fraction = status->epoch_fraction;
    request->revision = status->revision;
}

int an_watch_resync(const struct ANStatus *previous, const struct ANResponse *reply)
{
    return reply->result != AN_OK || !previous->revision ||
           previous->epoch != reply->status.epoch ||
           previous->epoch_fraction != reply->status.epoch_fraction ||
           reply->status.revision < previous->revision ||
           reply->status.revision - previous->revision > 1 ||
           ((reply->status.capabilities & AN_CAP_RESUMABLE_WATCH) && reply->more == AN_WATCH_RESYNC);
}

void an_request(struct ANRequest *request, enum ANOperation operation)
{
    memset(request, 0, sizeof(*request));
    request->operation = operation;
}

int an_submit(struct ANClient *client, const struct ANRequest *request)
{
    struct ANMessage *message;
    struct MsgPort *service;
    struct Node *node;
    unsigned queued = 0;
    int result = AN_UNAVAILABLE;

    if (client->pending)
        return AN_BUSY;
    if (!client->reply)
        client->reply = CreateMsgPort();
    if (!client->reply)
        return AN_BUSY;
    message = AllocVec(sizeof(*message), MEMF_PUBLIC | MEMF_CLEAR);
    if (!message)
        return AN_BUSY;
    message->message.mn_ReplyPort = client->reply;
    message->message.mn_Length = sizeof(*message);
    message->magic = AN_MAGIC;
    message->version = AN_VERSION;
    message->phase = AN_QUEUED;
    message->request = *request;

    Forbid();
    service = FindPort(AN_PORT);
    if (service) {
        for (node = service->mp_MsgList.lh_Head; node->ln_Succ; node = node->ln_Succ)
            ++queued;
        result = AN_BUSY;
        if (queued < AN_QUEUE_LIMIT) {
            client->pending = message;
            PutMsg(service, &message->message);
            result = AN_OK;
        }
    }
    Permit();
    if (result != AN_OK)
        FreeVec(message);
    return result;
}

int an_receive(struct ANClient *client, struct ANResponse *response)
{
    struct ANMessage *message;
    if (!client->pending)
        return 0;
    message = (struct ANMessage *)GetMsg(client->reply);
    if (!message || message != client->pending)
        return 0;
    if (message->message.mn_Length != sizeof(*message) ||
        message->magic != AN_MAGIC || message->version != AN_VERSION || message->phase != AN_REPLIED) {
        memset(response, 0, sizeof(*response));
        response->result = AN_VERSION_ERROR;
    } else {
        *response = message->response;
    }
    client->pending = NULL;
    FreeVec(message);
    return 1;
}

static void cancel(struct ANClient *client)
{
    struct ANMessage *message = client->pending;
    if (!message)
        return;
    /* The service handles native messages and its watch list under Forbid. */
    Forbid();
    Remove(&message->message.mn_Node);
    client->pending = NULL;
    Permit();
    FreeVec(message);
}

void an_close(struct ANClient *client)
{
    cancel(client);
    if (client->reply)
        DeleteMsgPort(client->reply);
    client->reply = NULL;
}

int an_call(struct ANClient *client, const struct ANRequest *request,
            struct ANResponse *response, unsigned ticks)
{
    int result;
    memset(response, 0, sizeof(*response));
    result = an_submit(client, request);
    if (result != AN_OK)
        return result;
    for (;;) {
        if (an_receive(client, response))
            return response->result;
        if (!ticks)
            break;
        Delay(1);
        --ticks;
    }
    cancel(client);
    return AN_TIMEOUT;
}
