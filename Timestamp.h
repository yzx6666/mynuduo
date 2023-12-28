#pragma once

#include <iostream>

// Ê±¼äÀà
class Timestamp
{
private:
    int64_t microSecondsSinceEpoch_;
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;

    static const int kMicroSecondsPerSecond = 1000 * 1000;

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

    static Timestamp invalid()
    {
        return Timestamp();
    }
    bool valid() const { return microSecondsSinceEpoch_ > 0; }
};

inline Timestamp addTime(Timestamp timestamp, double seconds)
{
  int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
  return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}