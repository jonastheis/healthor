//
// Generated file, do not edit! Created by nedtool 5.6 from health_message.msg.
//

#ifndef __HEALTH_MESSAGE_M_H
#define __HEALTH_MESSAGE_M_H

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
 * Class generated from <tt>health_message.msg:2</tt> by nedtool.
 * <pre>
 * message HealthMessage
 * {
 *     double health = 0;
 * }
 * </pre>
 */
class HealthMessage : public ::omnetpp::cMessage
{
  protected:
    double health;

  private:
    void copy(const HealthMessage& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const HealthMessage&);

  public:
    HealthMessage(const char *name=nullptr, short kind=0);
    HealthMessage(const HealthMessage& other);
    virtual ~HealthMessage();
    HealthMessage& operator=(const HealthMessage& other);
    virtual HealthMessage *dup() const override {return new HealthMessage(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual double getHealth() const;
    virtual void setHealth(double health);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const HealthMessage& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, HealthMessage& obj) {obj.parsimUnpack(b);}


#endif // ifndef __HEALTH_MESSAGE_M_H
