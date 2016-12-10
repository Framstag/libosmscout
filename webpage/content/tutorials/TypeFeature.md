---
date: "2016-09-15T19:25:58+02:00"
title:  "Creating new type features"
description: "How to create new type features"
weight: 3

menu:
  main:
    Parent: "tutorials"
    Weight: 3
---

## Goal of the tutorial

This tutorial show hows to create a new type feature. Type feature are used
to extent the internal database with additional information available
in the OSM data. Feature represent the existance of certain tags or 
tags combinations optionally together with their values.

Some examples for features are the value of tags like:

* [website](https://wiki.openstreetmap.org/wiki/Key:website)
* [phone](https://wiki.openstreetmap.org/wiki/Key:phone)
* [Opening hours](https://wiki.openstreetmap.org/wiki/Key:opening_hours)

Of course any other tag or combination of tags can in principle get
converted to a type feature.

For a more detailed  description regarding assigning features to types, see
[Type Definition]({{< relref "documentation/typedef.md" >}}).

## Required C++ headers and base classes

The base classes required for implementing type features can be found in
in the header `<osmscout/TypeConfig.h>`, concrete implementations
are placed in `<osmscout/TypeFeatures.h>`. Example for a valueless
feature is `osmscout::TunnelFeature`, simple example for a type feature
also holding a value is `NameFeature`.

The relevant bases classes for implementing a new feature are:

`osmscout::Feature` and `osmsocut::FeatureRef`
: This is the base class that holds meta information about the feature ands
  contains methods for parsing tags to the feature.

`osmscout::FeatureValue`
: Derived instances of this class hold the actual value of the feature
  (if the feature has a value) and also offers methofs for reading
  and writing feature values.

`osmscout::FeatureInstance`
: This is an instance of a feature for a given type. This contains some
  type specific information of the feature for a given type, like the actual
  bit position of the feature flag in the feature bit mask. You do not
  need to derive this class for a new feature.

`osmscout::FeatureValueBuffer`
: The FeatureValueBuffer handles access to the types and all the features of an
  given object instance. You do not need to derive this class for a new feature.

## Registering a feature

Before you be able to use a feature in the `OST` file and later on read and write
instances of it during import and later usage you first have to register it
to the type configuration. For this you have to call
`TypeConfig::RegisterFeature()` on a type config instance.

Note that you need to call above method during import and later on if you are
using the resulting database. You should also make sure that features are registered
in the same order in both cases.

Note that registering a feature does not cost you anything. So it makes sense
to make sure that your feature is pushed to upstream and automatically registered
by the library.

## Extending `osmscout::Feature` to define a new valueless feature

To define a new feature you need to extend the class `ossmcout::Feature`.

See for example the class `osmscout::TunnelFeature`, which implements
the feature `Tunnel` based on the tag
[tunnel](https://wiki.openstreetmap.org/wiki/Key:tunnel).

```c++
class OSMSCOUT_API TunnelFeature : public Feature
{
private:
  TagId tagTunnel;

public:
  /** Name of this feature */
  static const char* const NAME;

public:
  void Initialize(TypeConfig& typeConfig);

  std::string GetName() const;

  void Parse(Progress& progress,
             const TypeConfig& typeConfig,
             const FeatureInstance& feature,
             const ObjectOSMRef& object,
             const TagMap& tags,
             FeatureValueBuffer& buffer) const;
};
```

After a featured is created and registered to the `TypeConfig` instance, the
`initialize()` method is called, passing the actual `TypeConfig` instance.
 If you need the `TagId` of any tags, this is the place to get them from the 
 `TypeConfig` and save them. The `TunnelFeature` does this for the Tag
 `tunnel`:
 
```c++ 
void TunnelFeature::Initialize(TypeConfig& typeConfig)
{
  tagTunnel=typeConfig.RegisterTag("tunnel");
}
```
 
 The method `GetName()` has to be implemented to return the name of the feature
 as to be used in the `OST` file. It makes sense to define a constant for the
 name of the feature.
 
```c++
const char* const TunnelFeature::NAME = "Tunnel";

std::string TunnelFeature::GetName() const
{
  return NAME;
}
 ```
 
 Finally (for valueless features) you need to implement the `Parse` method
 to analyze the given object with the given map of tags for the existance of the
 feature:
 
```c++
void TunnelFeature::Parse(Progress& /*progress*/,
                          const TypeConfig& /*typeConfig*/,
                          const FeatureInstance& feature,
                          const ObjectOSMRef& /*object*/,
                          const TagMap& tags,
                          FeatureValueBuffer& buffer) const
{
  auto tunnel=tags.find(tagTunnel);

  if (tunnel!=tags.end() &&
      !(tunnel->second=="no" ||
        tunnel->second=="false" ||
        tunnel->second=="0")) {
    buffer.AllocateValue(feature.GetIndex());
  }
}
```
 If the feature exists, you have to allocate a value for it at the index
 as defined (and managed by the `TypeConfig`) by the passed 
 `FeatureInstance`.
 
 Since the base class has the following (overwriteable) definitions:
 
```c++
/**
 * A feature, if set for an object, can hold a value. If there is no value object,
 * this method returns 0, else it returns the C++ size of the value object.
 */
inline virtual size_t GetValueSize() const
{
  return 0;
}

/**
 * This method returns the number of additional feature bits reserved. If there are
 * additional features bit, 0 is returned.
 *
 * A feature may reserve additional feature bits. Feature bits should be used
 * if a custom value object is too expensive. Space for feature bits is always reserved
 * even if the feature itself is not set for a certain object.
 */
inline virtual size_t GetFeatureBitCount() const
{
  return 0;
}

/**
 * Returns 'true' if the feature has an value object.
 */
inline virtual bool HasValue() const
{
  return GetValueSize()>0;
}
```

There is no actual value for the feature, since the value size is 0. In this case
you are done.

## How to implement a feature with a value

For a feature also having a value we have to do further coding. Let`s look
at the `NameFeature`. First the `NameFeatureValue`.

```c++
class OSMSCOUT_API NameFeatureValue : public FeatureValue
{
private:
  std::string name;

public:
  inline NameFeatureValue()
  {
    // no code
  }

  inline NameFeatureValue(const std::string& name)
  : name(name)
  {
    // no code
  }

  inline void SetName(const std::string& name)
  {
    this->name=name;
  }

  inline std::string GetName() const
  {
    return name;
  }

  inline std::string GetLabel() const
  {
    return name;
  }

  void Read(FileScanner& scanner);
  void Write(FileWriter& writer);

  FeatureValue& operator=(const FeatureValue& other);
  bool operator==(const FeatureValue& other) const;
};
```

The `NameFeatureValue` extends `FeatureValue` overwriting the `GetLabel()` method,
the assignment and comparison methods and the `Read()` and `Write()` methods.

In this case the actzul value is the attribute `name` of type `std::string`.
The corresponding ovwrriten methods are thus as expected:

```c++
void NameFeatureValue::Read(FileScanner& scanner)
{
  scanner.Read(name);
}

void NameFeatureValue::Write(FileWriter& writer)
{
  writer.Write(name);
}
```

The `NameFeature` also offers additional `SetName()` and `GetName()` helper
methods and a constructor with the name as parameter.

If the feature itself has registered a label using the `RegisterLabel` base method
the `GetLabel()` method should be implemented to return a readable and renderable
representation of the feature. In this case we just return the name itself.

To signal the engine that a feature has a value the following methods of the
base class have to be implemented as follows:

```c++
size_t NameFeature::GetValueSize() const
{
  return sizeof(NameFeatureValue);
}

FeatureValue* NameFeature::AllocateValue(void* buffer)
{
  return new (buffer) NameFeatureValue();
}
```

In this case the size of the value is >0 and AllocateValue uses a special variant
of new to create a feature value instance at the given buffer position. The
buffer was before allocated to be big enough to hold all values of
all feature an actual object instance (`FeatureValueBuffer`) has.

## Additional feature bits

Using the method `GetFeatureBitCount()` you can request further feature bits
to be reserver for you. This is helpfull, if the feature value is
smaller than one byte. Not though that space is reserved for this additonal bit
always. So if if an object does not have a feature, the additional bit
is still allocated on disk and in memory.

You have to access the addtional bit manually. That means to have to find the index
of the feature in the feature bit field and get/set the additonal bits which are
directly paced behind the feature bit itself manually.
