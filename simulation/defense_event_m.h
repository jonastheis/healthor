//
// Generated file, do not edit! Created by nedtool 5.6 from defense_event.msg.
//

#ifndef __DEFENSE_EVENT_M_H
#define __DEFENSE_EVENT_M_H

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
 * Class generated from <tt>defense_event.msg:3</tt> by nedtool.
 * <pre>
 * message DefenseEvent
 * {
 * }
 * </pre>
 */
class DefenseEvent : public ::omnetpp::cMessage
{
  protected:

  private:
    void copy(const DefenseEvent& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const DefenseEvent&);

  public:
    DefenseEvent(const char *name=nullptr, short kind=0);
    DefenseEvent(const DefenseEvent& other);
    virtual ~DefenseEvent();
    DefenseEvent& operator=(const DefenseEvent& other);
    virtual DefenseEvent *dup() const override {return new DefenseEvent(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const DefenseEvent& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, DefenseEvent& obj) {obj.parsimUnpack(b);}


#endif // ifndef __DEFENSE_EVENT_M_H

