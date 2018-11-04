/*
 MIT License
 
 Copyright (c) 2016 kbinani
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <utility> // std::move, std::swap

template<class T>
class cfref_ptr
{
public:
    cfref_ptr()
    : obj_(NULL)
    {}
    
    cfref_ptr(T obj)
    : obj_(obj)
    {
    }
    
    ~cfref_ptr()
    {
        if (obj_) {
            CFRelease(obj_);
        }
    }
    
    cfref_ptr(cfref_ptr const& other)
    : obj_(other.obj_)
    {
        if (obj_) {
            CFRetain(obj_);
        }
    }
    
    cfref_ptr(cfref_ptr&& other) noexcept
    {
        *this = std::move(other);
    }
    
    cfref_ptr& operator = (cfref_ptr const& other)
    {
        if (this != &other) {
            cfref_ptr tmp(other);
            tmp.swap(*this);
        }
        return *this;
    }
    
    cfref_ptr& operator = (cfref_ptr&& other)
    {
        if (this != &other) {
            obj_ = other.obj_;
            other.obj_ = NULL;
        }
        return *this;
    }
    
    bool boolean_test() const
    {
        return !!obj_;
    }
    
    T get()
    {
        return obj_;
    }
    
    T release()
    {
        T ret = obj_;
        obj_ = NULL;
        return ret;
    }
    
    void swap(cfref_ptr& other)
    {
        std::swap(obj_, other.obj_);
    }
    
    void reset(T obj = NULL)
    {
        if (obj_) {
            CFRelease(obj_);
        }
        obj_ = obj;
    }
    
private:
    T obj_;
};
