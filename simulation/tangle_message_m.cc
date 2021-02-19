//
// Generated file, do not edit! Created by nedtool 5.6 from tangle_message.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include "tangle_message_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp


// forward
template<typename T, typename A>
std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec);

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
inline std::ostream& operator<<(std::ostream& out,const T&) {return out;}

// operator<< for std::vector<T>
template<typename T, typename A>
inline std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec)
{
    out.put('{');
    for(typename std::vector<T,A>::const_iterator it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin()) {
            out.put(','); out.put(' ');
        }
        out << *it;
    }
    out.put('}');
    
    char buf[32];
    sprintf(buf, " (size=%u)", (unsigned int)vec.size());
    out.write(buf, strlen(buf));
    return out;
}

Register_Class(TangleMessage)

TangleMessage::TangleMessage(const char *name, short kind) : ::omnetpp::cMessage(name,kind)
{
    this->issuerNode = 0;
    this->sequence = 0;
    this->issuingTime = 0;
    this->requestMessageResponse = false;
    this->processingTime = 0;
    this->solid = false;
}

TangleMessage::TangleMessage(const TangleMessage& other) : ::omnetpp::cMessage(other)
{
    copy(other);
}

TangleMessage::~TangleMessage()
{
}

TangleMessage& TangleMessage::operator=(const TangleMessage& other)
{
    if (this==&other) return *this;
    ::omnetpp::cMessage::operator=(other);
    copy(other);
    return *this;
}

void TangleMessage::copy(const TangleMessage& other)
{
    this->id = other.id;
    this->issuerNode = other.issuerNode;
    this->sequence = other.sequence;
    this->issuingTime = other.issuingTime;
    this->parent1 = other.parent1;
    this->parent2 = other.parent2;
    this->requestMessageResponse = other.requestMessageResponse;
    this->processingTime = other.processingTime;
    this->solid = other.solid;
}

void TangleMessage::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cMessage::parsimPack(b);
    doParsimPacking(b,this->id);
    doParsimPacking(b,this->issuerNode);
    doParsimPacking(b,this->sequence);
    doParsimPacking(b,this->issuingTime);
    doParsimPacking(b,this->parent1);
    doParsimPacking(b,this->parent2);
    doParsimPacking(b,this->requestMessageResponse);
    doParsimPacking(b,this->processingTime);
    doParsimPacking(b,this->solid);
}

void TangleMessage::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cMessage::parsimUnpack(b);
    doParsimUnpacking(b,this->id);
    doParsimUnpacking(b,this->issuerNode);
    doParsimUnpacking(b,this->sequence);
    doParsimUnpacking(b,this->issuingTime);
    doParsimUnpacking(b,this->parent1);
    doParsimUnpacking(b,this->parent2);
    doParsimUnpacking(b,this->requestMessageResponse);
    doParsimUnpacking(b,this->processingTime);
    doParsimUnpacking(b,this->solid);
}

const char * TangleMessage::getId() const
{
    return this->id.c_str();
}

void TangleMessage::setId(const char * id)
{
    this->id = id;
}

int TangleMessage::getIssuerNode() const
{
    return this->issuerNode;
}

void TangleMessage::setIssuerNode(int issuerNode)
{
    this->issuerNode = issuerNode;
}

int TangleMessage::getSequence() const
{
    return this->sequence;
}

void TangleMessage::setSequence(int sequence)
{
    this->sequence = sequence;
}

int64_t TangleMessage::getIssuingTime() const
{
    return this->issuingTime;
}

void TangleMessage::setIssuingTime(int64_t issuingTime)
{
    this->issuingTime = issuingTime;
}

const char * TangleMessage::getParent1() const
{
    return this->parent1.c_str();
}

void TangleMessage::setParent1(const char * parent1)
{
    this->parent1 = parent1;
}

const char * TangleMessage::getParent2() const
{
    return this->parent2.c_str();
}

void TangleMessage::setParent2(const char * parent2)
{
    this->parent2 = parent2;
}

bool TangleMessage::getRequestMessageResponse() const
{
    return this->requestMessageResponse;
}

void TangleMessage::setRequestMessageResponse(bool requestMessageResponse)
{
    this->requestMessageResponse = requestMessageResponse;
}

int64_t TangleMessage::getProcessingTime() const
{
    return this->processingTime;
}

void TangleMessage::setProcessingTime(int64_t processingTime)
{
    this->processingTime = processingTime;
}

bool TangleMessage::getSolid() const
{
    return this->solid;
}

void TangleMessage::setSolid(bool solid)
{
    this->solid = solid;
}

class TangleMessageDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
  public:
    TangleMessageDescriptor();
    virtual ~TangleMessageDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyname) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyname) const override;
    virtual int getFieldArraySize(void *object, int field) const override;

    virtual const char *getFieldDynamicTypeString(void *object, int field, int i) const override;
    virtual std::string getFieldValueAsString(void *object, int field, int i) const override;
    virtual bool setFieldValueAsString(void *object, int field, int i, const char *value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual void *getFieldStructValuePointer(void *object, int field, int i) const override;
};

Register_ClassDescriptor(TangleMessageDescriptor)

TangleMessageDescriptor::TangleMessageDescriptor() : omnetpp::cClassDescriptor("TangleMessage", "omnetpp::cMessage")
{
    propertynames = nullptr;
}

TangleMessageDescriptor::~TangleMessageDescriptor()
{
    delete[] propertynames;
}

bool TangleMessageDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<TangleMessage *>(obj)!=nullptr;
}

const char **TangleMessageDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *TangleMessageDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int TangleMessageDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 9+basedesc->getFieldCount() : 9;
}

unsigned int TangleMessageDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
    };
    return (field>=0 && field<9) ? fieldTypeFlags[field] : 0;
}

const char *TangleMessageDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldNames[] = {
        "id",
        "issuerNode",
        "sequence",
        "issuingTime",
        "parent1",
        "parent2",
        "requestMessageResponse",
        "processingTime",
        "solid",
    };
    return (field>=0 && field<9) ? fieldNames[field] : nullptr;
}

int TangleMessageDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount() : 0;
    if (fieldName[0]=='i' && strcmp(fieldName, "id")==0) return base+0;
    if (fieldName[0]=='i' && strcmp(fieldName, "issuerNode")==0) return base+1;
    if (fieldName[0]=='s' && strcmp(fieldName, "sequence")==0) return base+2;
    if (fieldName[0]=='i' && strcmp(fieldName, "issuingTime")==0) return base+3;
    if (fieldName[0]=='p' && strcmp(fieldName, "parent1")==0) return base+4;
    if (fieldName[0]=='p' && strcmp(fieldName, "parent2")==0) return base+5;
    if (fieldName[0]=='r' && strcmp(fieldName, "requestMessageResponse")==0) return base+6;
    if (fieldName[0]=='p' && strcmp(fieldName, "processingTime")==0) return base+7;
    if (fieldName[0]=='s' && strcmp(fieldName, "solid")==0) return base+8;
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *TangleMessageDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "string",
        "int",
        "int",
        "int64_t",
        "string",
        "string",
        "bool",
        "int64_t",
        "bool",
    };
    return (field>=0 && field<9) ? fieldTypeStrings[field] : nullptr;
}

const char **TangleMessageDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldPropertyNames(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *TangleMessageDescriptor::getFieldProperty(int field, const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldProperty(field, propertyname);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int TangleMessageDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    TangleMessage *pp = (TangleMessage *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

const char *TangleMessageDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    TangleMessage *pp = (TangleMessage *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string TangleMessageDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    TangleMessage *pp = (TangleMessage *)object; (void)pp;
    switch (field) {
        case 0: return oppstring2string(pp->getId());
        case 1: return long2string(pp->getIssuerNode());
        case 2: return long2string(pp->getSequence());
        case 3: return int642string(pp->getIssuingTime());
        case 4: return oppstring2string(pp->getParent1());
        case 5: return oppstring2string(pp->getParent2());
        case 6: return bool2string(pp->getRequestMessageResponse());
        case 7: return int642string(pp->getProcessingTime());
        case 8: return bool2string(pp->getSolid());
        default: return "";
    }
}

bool TangleMessageDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->setFieldValueAsString(object,field,i,value);
        field -= basedesc->getFieldCount();
    }
    TangleMessage *pp = (TangleMessage *)object; (void)pp;
    switch (field) {
        case 0: pp->setId((value)); return true;
        case 1: pp->setIssuerNode(string2long(value)); return true;
        case 2: pp->setSequence(string2long(value)); return true;
        case 3: pp->setIssuingTime(string2int64(value)); return true;
        case 4: pp->setParent1((value)); return true;
        case 5: pp->setParent2((value)); return true;
        case 6: pp->setRequestMessageResponse(string2bool(value)); return true;
        case 7: pp->setProcessingTime(string2int64(value)); return true;
        case 8: pp->setSolid(string2bool(value)); return true;
        default: return false;
    }
}

const char *TangleMessageDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructName(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

void *TangleMessageDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    TangleMessage *pp = (TangleMessage *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}


