#ifndef INTERLEAVED_FUTURE_H 
#define INTERLEAVED_FUTURE_H

#include <QFuture>
#include <QtConcurrent>


template <class T, T(*F)()>
struct InterleavedFuture {
    QFuture<T> f = QtConcurrent::run(F);

    T operator()() {
        auto res = f.result();
        f = QtConcurrent::run(F);
        return res;
    }
};


#endif