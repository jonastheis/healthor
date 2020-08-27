//
// Generated file, do not edit! Created by nedtool 5.6 from processing_event.msg.
//

#ifndef __PROCESSING_EVENT_M_H
#define __PROCESSING_EVENT_M_H

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
 * Class generated from <tt>processing_event.msg:3</tt> by nedtool.
 * <pre>
 * message ProcessingEvent
 * {
 * }
 * </pre>
 */
class ProcessingEvent : public ::omnetpp::cMessage
{
  protected:

  private:
    void copy(const ProcessingEvent& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const ProcessingEvent&);

  public:
    ProcessingEvent(const char *name=nullptr, short kind=0);
    ProcessingEvent(const ProcessingEvent& other);
    virtual ~ProcessingEvent();
    ProcessingEvent& operator=(const ProcessingEvent& other);
    virtual ProcessingEvent *dup() const override {return new ProcessingEvent(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const ProcessingEvent& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, ProcessingEvent& obj) {obj.parsimUnpack(b);}


#endif // ifndef __PROCESSING_EVENT_M_H
