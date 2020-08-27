//
// Generated file, do not edit! Created by nedtool 5.6 from generation_event.msg.
//

#ifndef __GENERATION_EVENT_M_H
#define __GENERATION_EVENT_M_H

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
 * Class generated from <tt>generation_event.msg:3</tt> by nedtool.
 * <pre>
 * message GenerationEvent
 * {
 * }
 * </pre>
 */
class GenerationEvent : public ::omnetpp::cMessage
{
  protected:

  private:
    void copy(const GenerationEvent& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const GenerationEvent&);

  public:
    GenerationEvent(const char *name=nullptr, short kind=0);
    GenerationEvent(const GenerationEvent& other);
    virtual ~GenerationEvent();
    GenerationEvent& operator=(const GenerationEvent& other);
    virtual GenerationEvent *dup() const override {return new GenerationEvent(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const GenerationEvent& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, GenerationEvent& obj) {obj.parsimUnpack(b);}


#endif // ifndef __GENERATION_EVENT_M_H

