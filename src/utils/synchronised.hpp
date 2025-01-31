//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2010-2015 Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef HEADER_SYNCHRONISED_HPP
#define HEADER_SYNCHRONISED_HPP
#include <mutex>

class ISynchronised
{
public :
    virtual ~ISynchronised() {}
    virtual void lock() const = 0 ;
    virtual void unlock() const = 0;
};

/** A variable that is automatically synchronised using pthreads mutex.
 */
template<typename TYPE>
class Synchronised : public ISynchronised
{
private:
    /** The mutex to protect this variable with. */
    mutable std::mutex      m_mutex;
    /** The actual data to be used. */
    TYPE                     m_data;
public:
	Synchronised(const Synchronised& o) {
		m_data = o.m_data;
	}
    // ------------------------------------------------------------------------
    /** Initialise the data and the mutex with default constructors. */
    Synchronised() : m_data()
    {
    }   // Synchronised()

    // ------------------------------------------------------------------------
    /** Initialise the data and the mutex. */
    Synchronised(const TYPE &v)
    {
        m_data = v;
    }   // Synchronised

    // ------------------------------------------------------------------------
    /** Destroy this mutex.
     */
    ~Synchronised()
    {
    }   // ~Synchronised

    // ------------------------------------------------------------------------
    /** Sets the value of this variable using a mutex.
     *  \param v Value to be set.
     */
    void setAtomic(const TYPE &v)
    {
		std::lock_guard<std::mutex> lock(m_mutex);
        m_data = v;
    }   // set

    // ------------------------------------------------------------------------
    /** Returns a copy of this variable.
     */
    TYPE getAtomic() const
    {
		std::lock_guard<std::mutex> lock(m_mutex);
        return m_data;
    }   // get
    // ------------------------------------------------------------------------
    /** Returns a reference to the original data file. NOTE: all access to
     *  the data files happen without mutex protection, so calls to lock
     *  and unlock are necessary. This method is useful in cases that several
     *  operations on the class must happen atomic.
     */
    TYPE &getData()
    {
        return m_data;
    }   // getData
    // ------------------------------------------------------------------------
    const TYPE &getData() const
    {
        return m_data;
    }   // getData
    // ------------------------------------------------------------------------
    /** Locks the mutex. Note that calls to get() or set() will fail, since
     *  they will try to lock the mutex as well!
     */
    void lock() const { m_mutex.lock(); }
    // ------------------------------------------------------------------------
    /** Unlocks the mutex.
     */
    void unlock() const { m_mutex.unlock(); }
    // ------------------------------------------------------------------------
    /** Gives access to the mutex, which can then be used in other pthread
     *  calls (e.g. pthread_cond_wait).
     */
    std::mutex & getMutex() { return m_mutex; }
private:
    // Make sure that no actual copying is taking place
    // ------------------------------------------------------------------------
	Synchronised& operator=(const Synchronised<TYPE>&) = delete;
};

#define MutexLocker(x) MutexLockerHelper __dummy(x);

class MutexLockerHelper
{
    const ISynchronised * m_synchronised;
public:
    MutexLockerHelper(const ISynchronised & synchronised){
        m_synchronised = &synchronised;
        m_synchronised->lock();
    }

    ~MutexLockerHelper(){
        m_synchronised->unlock();
    }
};


#endif
