//
// Generated file, do not edit! Created by nedtool 5.6 from sending_event.msg.
//

#ifndef __SENDING_EVENT_M_H
#define __SENDING_EVENT_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0506
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



/**
 * Class generated from <tt>sending_event.msg:2</tt> by nedtool.
 * <pre>
 * message SendingEvent
 * {
 *     int neighbor = 0;
 * }
 * </pre>
 */
class SendingEvent : public ::omnetpp::cMessage
{
  protected:
    int neighbor;

  private:
    void copy(const SendingEvent& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const SendingEvent&);

  public:
    SendingEvent(const char *name=nullptr, short kind=0);
    SendingEvent(const SendingEvent& other);
    virtual ~SendingEvent();
    SendingEvent& operator=(const SendingEvent& other);
    virtual SendingEvent *dup() const override {return new SendingEvent(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual int getNeighbor() const;
    virtual void setNeighbor(int neighbor);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const SendingEvent& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, SendingEvent& obj) {obj.parsimUnpack(b);}


#endif // ifndef __SENDING_EVENT_M_H

