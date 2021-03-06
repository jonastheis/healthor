//
// Generated file, do not edit! Created by nedtool 5.6 from scheduling_event.msg.
//

#ifndef __SCHEDULING_EVENT_M_H
#define __SCHEDULING_EVENT_M_H

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
 * Class generated from <tt>scheduling_event.msg:3</tt> by nedtool.
 * <pre>
 * message SchedulingEvent
 * {
 * }
 * </pre>
 */
class SchedulingEvent : public ::omnetpp::cMessage
{
  protected:

  private:
    void copy(const SchedulingEvent& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const SchedulingEvent&);

  public:
    SchedulingEvent(const char *name=nullptr, short kind=0);
    SchedulingEvent(const SchedulingEvent& other);
    virtual ~SchedulingEvent();
    SchedulingEvent& operator=(const SchedulingEvent& other);
    virtual SchedulingEvent *dup() const override {return new SchedulingEvent(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const SchedulingEvent& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, SchedulingEvent& obj) {obj.parsimUnpack(b);}


#endif // ifndef __SCHEDULING_EVENT_M_H

