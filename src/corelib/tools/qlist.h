/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2019 Intel Corporation
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QLIST_H
#define QLIST_H

#include <QtCore/qarraydatapointer.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qhashfunctions.h>
#include <QtCore/qiterator.h>

#include <functional>
#include <limits>
#include <initializer_list>
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
   template <typename V, typename U> qsizetype indexOf(const QList<V> &list, const U &u, qsizetype from) noexcept;
   template <typename V, typename U> qsizetype lastIndexOf(const QList<V> &list, const U &u, qsizetype from) noexcept;
}

template <typename T> struct QListSpecialMethodsBase
{
protected:
    ~QListSpecialMethodsBase() = default;

    using Self = QList<T>;
    Self *self() { return static_cast<Self *>(this); }
    const Self *self() const { return static_cast<const Self *>(this); }

public:
    template <typename AT = T>
    qsizetype indexOf(const AT &t, qsizetype from = 0) const noexcept;
    template <typename AT = T>
    qsizetype lastIndexOf(const AT &t, qsizetype from = -1) const noexcept;

    template <typename AT = T>
    bool contains(const AT &t) const noexcept
    {
        return self()->indexOf(t) != -1;
    }
};
template <typename T> struct QListSpecialMethods : QListSpecialMethodsBase<T>
{
protected:
    ~QListSpecialMethods() = default;
public:
    using QListSpecialMethodsBase<T>::indexOf;
    using QListSpecialMethodsBase<T>::lastIndexOf;
    using QListSpecialMethodsBase<T>::contains;
};
template <> struct QListSpecialMethods<QByteArray>;
template <> struct QListSpecialMethods<QString>;

#ifdef Q_QDOC // define QVector for QDoc
template<typename T> class QVector : public QList<T> {};
#endif

template <typename T>
class QList
#ifndef Q_QDOC
    : public QListSpecialMethods<T>
#endif
{
    using Data = QTypedArrayData<T>;
    using DataOps = QArrayDataOps<T>;
    using DataPointer = QArrayDataPointer<T>;
    class DisableRValueRefs {};

    DataPointer d;

    template <typename V, typename U> friend qsizetype QtPrivate::indexOf(const QList<V> &list, const U &u, qsizetype from) noexcept;
    template <typename V, typename U> friend qsizetype QtPrivate::lastIndexOf(const QList<V> &list, const U &u, qsizetype from) noexcept;

public:
    using Type = T;
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = qsizetype;
    using difference_type = qptrdiff;
#ifndef Q_QDOC
    using iterator = typename Data::iterator;
    using const_iterator = typename Data::const_iterator;
#else  // simplified aliases for QDoc
    using iterator = T *;
    using const_iterator = const T *;
#endif
    using Iterator = iterator;
    using ConstIterator = const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
#ifndef Q_QDOC
    using parameter_type = typename DataPointer::parameter_type;
    using rvalue_ref = typename std::conditional<DataPointer::pass_parameter_by_value, DisableRValueRefs, T &&>::type;
#else  // simplified aliases for QDoc
    using parameter_type = const T &;
    using rvalue_ref = T &&;
#endif

private:
    void resize_internal(qsizetype i);
    bool isValidIterator(const_iterator i) const
    {
        const std::less<const T*> less = {};
        return !less(d->end(), i) && !less(i, d->begin());
    }
public:
    QList(DataPointer dd) noexcept
        : d(dd)
    {
    }

public:
    QList() = default;
    explicit QList(qsizetype size)
        : d(Data::allocate(size))
    {
        if (size)
            d->appendInitialize(size);
    }
    QList(qsizetype size, parameter_type t)
        : d(Data::allocate(size))
    {
        if (size)
            d->copyAppend(size, t);
    }

    inline QList(std::initializer_list<T> args)
        : d(Data::allocate(args.size()))
    {
        if (args.size())
            d->copyAppend(args.begin(), args.end());
    }

    QList<T> &operator=(std::initializer_list<T> args)
    {
        d = DataPointer(Data::allocate(args.size()));
        if (args.size())
            d->copyAppend(args.begin(), args.end());
        return *this;
    }
    template <typename InputIterator, QtPrivate::IfIsForwardIterator<InputIterator> = true>
    QList(InputIterator i1, InputIterator i2)
    {
        const auto distance = std::distance(i1, i2);
        if (distance) {
            d = DataPointer(Data::allocate(distance));
            d->copyAppend(i1, i2);
        }
    }

    template <typename InputIterator, QtPrivate::IfIsNotForwardIterator<InputIterator> = true>
    QList(InputIterator i1, InputIterator i2)
    {
        std::copy(i1, i2, std::back_inserter(*this));
    }

    // This constructor is here for compatibility with QStringList in Qt 5, that has a QStringList(const QString &) constructor
    template<typename String, typename = std::enable_if_t<std::is_same_v<T, QString> && std::is_convertible_v<String, QString>>>
    inline explicit QList(const String &str)
    { append(str); }

    // compiler-generated special member functions are fine!

    void swap(QList<T> &other) noexcept { qSwap(d, other.d); }

    template <typename U = T>
    QTypeTraits::compare_eq_result<U> operator==(const QList &other) const
    {
        if (size() != other.size())
            return false;
        if (begin() == other.begin())
            return true;

        // do element-by-element comparison
        return d->compare(begin(), other.begin(), size());
    }
    template <typename U = T>
    QTypeTraits::compare_eq_result<U> operator!=(const QList &other) const
    {
        return !(*this == other);
    }

    template <typename U = T>
    QTypeTraits::compare_lt_result<U> operator<(const QList &other) const
        noexcept(noexcept(std::lexicographical_compare<typename QList<U>::const_iterator,
                                                       typename QList::const_iterator>(
                            std::declval<QList<U>>().begin(), std::declval<QList<U>>().end(),
                            other.begin(), other.end())))
    {
        return std::lexicographical_compare(begin(), end(),
                                            other.begin(), other.end());
    }

    template <typename U = T>
    QTypeTraits::compare_lt_result<U> operator>(const QList &other) const
        noexcept(noexcept(other < std::declval<QList<U>>()))
    {
        return other < *this;
    }

    template <typename U = T>
    QTypeTraits::compare_lt_result<U> operator<=(const QList &other) const
        noexcept(noexcept(other < std::declval<QList<U>>()))
    {
        return !(other < *this);
    }

    template <typename U = T>
    QTypeTraits::compare_lt_result<U> operator>=(const QList &other) const
        noexcept(noexcept(std::declval<QList<U>>() < other))
    {
        return !(*this < other);
    }

    qsizetype size() const noexcept { return d->size; }
    qsizetype count() const noexcept { return size(); }
    qsizetype length() const noexcept { return size(); }

    inline bool isEmpty() const noexcept { return d->size == 0; }

    void resize(qsizetype size)
    {
        resize_internal(size);
        if (size > this->size())
            d->appendInitialize(size);
    }
    void resize(qsizetype size, parameter_type c)
    {
        resize_internal(size);
        if (size > this->size())
            d->copyAppend(size - this->size(), c);
    }

    inline qsizetype capacity() const { return qsizetype(d->constAllocatedCapacity()); }
    void reserve(qsizetype size);
    inline void squeeze();

    void detach() { d.detach(); }
    bool isDetached() const noexcept { return !d->isShared(); }

    inline bool isSharedWith(const QList<T> &other) const { return d == other.d; }

    pointer data() { detach(); return d->data(); }
    const_pointer data() const noexcept { return d->data(); }
    const_pointer constData() const noexcept { return d->data(); }
    void clear() {
        if (!size())
            return;
        if (d->needsDetach()) {
            // must allocate memory
            DataPointer detached(Data::allocate(d.allocatedCapacity()));
            d.swap(detached);
        } else {
            d->truncate(0);
        }
    }

    const_reference at(qsizetype i) const noexcept
    {
        Q_ASSERT_X(size_t(i) < size_t(d->size), "QList::at", "index out of range");
        return data()[i];
    }
    reference operator[](qsizetype i)
    {
        Q_ASSERT_X(size_t(i) < size_t(d->size), "QList::operator[]", "index out of range");
        detach();
        return data()[i];
    }
    const_reference operator[](qsizetype i) const noexcept { return at(i); }
    void append(parameter_type t) { emplaceBack(t); }
    void append(const_iterator i1, const_iterator i2);
    void append(rvalue_ref t) { emplaceBack(std::move(t)); }
    void append(const QList<T> &l)
    {
        // protect against l == *this
        QList list(l);
        append(list.constBegin(), list.constEnd());
    }
    void append(QList<T> &&l);
    void prepend(rvalue_ref t) { emplaceFront(std::move(t)); }
    void prepend(parameter_type t) { emplaceFront(t); }

    template<typename... Args>
    inline reference emplaceBack(Args &&... args);

    template <typename ...Args>
    inline reference emplaceFront(Args&&... args);

    iterator insert(qsizetype i, parameter_type t)
    { return insert(i, 1, t); }
    iterator insert(qsizetype i, qsizetype n, parameter_type t);
    iterator insert(const_iterator before, parameter_type t)
    {
        Q_ASSERT_X(isValidIterator(before),  "QList::insert", "The specified iterator argument 'before' is invalid");
        return insert(before, 1, t);
    }
    iterator insert(const_iterator before, qsizetype n, parameter_type t)
    {
        Q_ASSERT_X(isValidIterator(before),  "QList::insert", "The specified iterator argument 'before' is invalid");
        return insert(std::distance(constBegin(), before), n, t);
    }
    iterator insert(const_iterator before, rvalue_ref t)
    {
        Q_ASSERT_X(isValidIterator(before),  "QList::insert", "The specified iterator argument 'before' is invalid");
        return insert(std::distance(constBegin(), before), std::move(t));
    }
    iterator insert(qsizetype i, rvalue_ref t) { return emplace(i, std::move(t)); }

    template <typename ...Args>
    iterator emplace(const_iterator before, Args&&... args)
    {
        Q_ASSERT_X(isValidIterator(before),  "QList::emplace", "The specified iterator argument 'before' is invalid");
        return emplace(std::distance(constBegin(), before), std::forward<Args>(args)...);
    }

    template <typename ...Args>
    iterator emplace(qsizetype i, Args&&... args);
#if 0
    template< class InputIt >
    iterator insert( const_iterator pos, InputIt first, InputIt last );
    iterator insert( const_iterator pos, std::initializer_list<T> ilist );
#endif
    void replace(qsizetype i, parameter_type t)
    {
        Q_ASSERT_X(i >= 0 && i < d->size, "QList<T>::replace", "index out of range");
        const T copy(t);
        data()[i] = copy;
    }
    void replace(qsizetype i, rvalue_ref t)
    {
        Q_ASSERT_X(i >= 0 && i < d->size, "QList<T>::replace", "index out of range");
        const T copy(std::move(t));
        data()[i] = std::move(copy);
    }

    void remove(qsizetype i, qsizetype n = 1);
    void removeFirst();
    void removeLast();
    value_type takeFirst() { Q_ASSERT(!isEmpty()); value_type v = std::move(first()); remove(0); return v; }
    value_type takeLast() { Q_ASSERT(!isEmpty()); value_type v = std::move(last()); remove(size() - 1); return v; }

    QList<T> &fill(parameter_type t, qsizetype size = -1);

#ifndef Q_QDOC
    using QListSpecialMethods<T>::contains;
    using QListSpecialMethods<T>::indexOf;
    using QListSpecialMethods<T>::lastIndexOf;
#else
    template <typename AT>
    qsizetype indexOf(const AT &t, qsizetype from = 0) const noexcept;
    template <typename AT>
    qsizetype lastIndexOf(const AT &t, qsizetype from = -1) const noexcept;
    template <typename AT>
    bool contains(const AT &t) const noexcept;
#endif

    template <typename AT = T>
    qsizetype count(const AT &t) const noexcept
    {
        return qsizetype(std::count(&*cbegin(), &*cend(), t));
    }

    // QList compatibility
    void removeAt(qsizetype i) { remove(i); }
    template <typename AT = T>
    qsizetype removeAll(const AT &t)
    {
        const const_iterator ce = this->cend(), cit = std::find(this->cbegin(), ce, t);
        if (cit == ce)
            return 0;
        qsizetype index = cit - this->cbegin();

        // Next operation detaches, so ce, cit may become invalidated.
        // Moreover -- unlike std::erase -- we do support the case where t
        // belongs to this list, so we have to save it from invalidation
        // by taking a copy. This is made slightly more complex by the fact
        // that t might not be copiable (in which case it certainly does not
        // belong to this list), in which case we just use the original.
        using CopyProxy = std::conditional_t<std::is_copy_constructible_v<AT>, AT, const AT &>;
        const AT &tCopy = CopyProxy(t);
        const iterator e = end(), it = std::remove(begin() + index, e, tCopy);
        const qsizetype result = std::distance(it, e);
        erase(it, e);
        return result;
    }
    template <typename AT = T>
    bool removeOne(const AT &t)
    {
        const qsizetype i = indexOf(t);
        if (i < 0)
            return false;
        remove(i);
        return true;
    }
    T takeAt(qsizetype i) { T t = std::move((*this)[i]); remove(i); return t; }
    void move(qsizetype from, qsizetype to)
    {
        Q_ASSERT_X(from >= 0 && from < size(), "QList::move(qsizetype, qsizetype)", "'from' is out-of-range");
        Q_ASSERT_X(to >= 0 && to < size(), "QList::move(qsizetype, qsizetype)", "'to' is out-of-range");
        if (from == to) // don't detach when no-op
            return;
        detach();
        T * const b = d->begin();
        if (from < to)
            std::rotate(b + from, b + from + 1, b + to + 1);
        else
            std::rotate(b + to, b + from, b + from + 1);
    }

    // STL-style
    iterator begin() { detach(); return d->begin(); }
    iterator end() { detach(); return d->end(); }

    const_iterator begin() const noexcept { return d->constBegin(); }
    const_iterator end() const noexcept { return d->constEnd(); }
    const_iterator cbegin() const noexcept { return d->constBegin(); }
    const_iterator cend() const noexcept { return d->constEnd(); }
    const_iterator constBegin() const noexcept { return d->constBegin(); }
    const_iterator constEnd() const noexcept { return d->constEnd(); }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

    iterator erase(const_iterator begin, const_iterator end);
    inline iterator erase(const_iterator pos) { return erase(pos, pos+1); }

    // more Qt
    inline T& first() { Q_ASSERT(!isEmpty()); return *begin(); }
    inline const T &first() const { Q_ASSERT(!isEmpty()); return *begin(); }
    inline const T &constFirst() const { Q_ASSERT(!isEmpty()); return *begin(); }
    inline T& last() { Q_ASSERT(!isEmpty()); return *(end()-1); }
    inline const T &last() const { Q_ASSERT(!isEmpty()); return *(end()-1); }
    inline const T &constLast() const { Q_ASSERT(!isEmpty()); return *(end()-1); }
    inline bool startsWith(parameter_type t) const { return !isEmpty() && first() == t; }
    inline bool endsWith(parameter_type t) const { return !isEmpty() && last() == t; }
    QList<T> mid(qsizetype pos, qsizetype len = -1) const;

    QList<T> first(qsizetype n) const
    {
        Q_ASSERT(size_t(n) <= size_t(size()));
        return QList<T>(begin(), begin() + n);
    }
    QList<T> last(qsizetype n) const
    {
        Q_ASSERT(size_t(n) <= size_t(size()));
        return QList<T>(end() - n, end());
    }
    QList<T> sliced(qsizetype pos) const
    {
        Q_ASSERT(size_t(pos) <= size_t(size()));
        return QList<T>(begin() + pos, end());
    }
    QList<T> sliced(qsizetype pos, qsizetype n) const
    {
        Q_ASSERT(size_t(pos) <= size_t(size()));
        Q_ASSERT(n >= 0);
        Q_ASSERT(pos + n <= size());
        return QList<T>(begin() + pos, begin() + pos + n);
    }

    T value(qsizetype i) const { return value(i, T()); }
    T value(qsizetype i, parameter_type defaultValue) const;

    void swapItemsAt(qsizetype i, qsizetype j) {
        Q_ASSERT_X(i >= 0 && i < size() && j >= 0 && j < size(),
                    "QList<T>::swap", "index out of range");
        detach();
        qSwap(d->begin()[i], d->begin()[j]);
    }

    // STL compatibility
    inline void push_back(parameter_type t) { append(t); }
    void push_back(rvalue_ref t) { append(std::move(t)); }
    void push_front(rvalue_ref t) { prepend(std::move(t)); }
    inline void push_front(parameter_type t) { prepend(t); }
    void pop_back() { removeLast(); }
    void pop_front() { removeFirst(); }

    template <typename ...Args>
    reference emplace_back(Args&&... args) { return emplaceBack(std::forward<Args>(args)...); }

    inline bool empty() const
    { return d->size == 0; }
    inline reference front() { return first(); }
    inline const_reference front() const { return first(); }
    inline reference back() { return last(); }
    inline const_reference back() const { return last(); }
    void shrink_to_fit() { squeeze(); }

    // comfort
    QList<T> &operator+=(const QList<T> &l) { append(l); return *this; }
    QList<T> &operator+=(QList<T> &&l) { append(std::move(l)); return *this; }
    inline QList<T> operator+(const QList<T> &l) const
    { QList n = *this; n += l; return n; }
    inline QList<T> operator+(QList<T> &&l) const
    { QList n = *this; n += std::move(l); return n; }
    inline QList<T> &operator+=(parameter_type t)
    { append(t); return *this; }
    inline QList<T> &operator<< (parameter_type t)
    { append(t); return *this; }
    inline QList<T> &operator<<(const QList<T> &l)
    { *this += l; return *this; }
    inline QList<T> &operator<<(QList<T> &&l)
    { *this += std::move(l); return *this; }
    inline QList<T> &operator+=(rvalue_ref t)
    { append(std::move(t)); return *this; }
    inline QList<T> &operator<<(rvalue_ref t)
    { append(std::move(t)); return *this; }

    // Consider deprecating in 6.4 or later
    static QList<T> fromList(const QList<T> &list) { return list; }
    QList<T> toList() const { return *this; }

    static inline QList<T> fromVector(const QList<T> &vector) { return vector; }
    inline QList<T> toVector() const { return *this; }

    template<qsizetype N>
    static QList<T> fromReadOnlyData(const T (&t)[N])
    {
        return QList<T>({ nullptr, const_cast<T *>(t), N });
    }
};

#if defined(__cpp_deduction_guides) && __cpp_deduction_guides >= 201606
template <typename InputIterator,
          typename ValueType = typename std::iterator_traits<InputIterator>::value_type,
          QtPrivate::IfIsInputIterator<InputIterator> = true>
QList(InputIterator, InputIterator) -> QList<ValueType>;
#endif

template <typename T>
inline void QList<T>::resize_internal(qsizetype newSize)
{
    Q_ASSERT(newSize >= 0);

    if (d->needsDetach() || newSize > capacity() - d.freeSpaceAtBegin()) {
        // must allocate memory
        DataPointer detached(Data::allocate(d->detachCapacity(newSize)));
        if (size() && newSize) {
            detached->copyAppend(constBegin(), constBegin() + qMin(newSize, size()));
        }
        d.swap(detached);
    }

    if (newSize < size())
        d->truncate(newSize);
}

template <typename T>
void QList<T>::reserve(qsizetype asize)
{
    // capacity() == 0 for immutable data, so this will force a detaching below
    if (asize <= capacity() - d.freeSpaceAtBegin()) {
        if (d->flags() & Data::CapacityReserved)
            return;  // already reserved, don't shrink
        if (!d->isShared()) {
            // accept current allocation, don't shrink
            d->setFlag(Data::CapacityReserved);
            return;
        }
    }

    DataPointer detached(Data::allocate(qMax(asize, size())));
    detached->copyAppend(constBegin(), constEnd());
    if (detached.d_ptr())
        detached->setFlag(Data::CapacityReserved);
    d.swap(detached);
}

template <typename T>
inline void QList<T>::squeeze()
{
    if (!d.isMutable())
        return;
    if (d->needsDetach() || size() < capacity()) {
        // must allocate memory
        DataPointer detached(Data::allocate(size()));
        if (size()) {
            detached->copyAppend(constBegin(), constEnd());
        }
        d.swap(detached);
    }
    // We're detached so this is fine
    d->clearFlag(Data::CapacityReserved);
}

template <typename T>
inline void QList<T>::remove(qsizetype i, qsizetype n)
{
    Q_ASSERT_X(size_t(i) + size_t(n) <= size_t(d->size), "QList::remove", "index out of range");
    Q_ASSERT_X(n >= 0, "QList::remove", "invalid count");

    if (n == 0)
        return;

    if (d->needsDetach())
        d.detach();

    d->erase(d->begin() + i, d->begin() + i + n);
}

template <typename T>
inline void QList<T>::removeFirst()
{
    Q_ASSERT(!isEmpty());
    if (d->needsDetach())
        d.detach();
    d->eraseFirst();
}

template <typename T>
inline void QList<T>::removeLast()
{
    Q_ASSERT(!isEmpty());
    if (d->needsDetach())
        detach();
    d->eraseLast();
}


template<typename T>
inline T QList<T>::value(qsizetype i, parameter_type defaultValue) const
{
    return size_t(i) < size_t(d->size) ? at(i) : defaultValue;
}

template <typename T>
inline void QList<T>::append(const_iterator i1, const_iterator i2)
{
    if (i1 == i2)
        return;
    const auto distance = std::distance(i1, i2);
    if (d->needsDetach() || distance > d.freeSpaceAtEnd()) {
        DataPointer detached(DataPointer::allocateGrow(d, distance, QArrayData::GrowsAtEnd));
        detached->copyAppend(constBegin(), constEnd());
        detached->copyAppend(i1, i2);
        d.swap(detached);
    } else {
        // we're detached and we can just move data around
        d->copyAppend(i1, i2);
    }
}

template <typename T>
inline void QList<T>::append(QList<T> &&other)
{
    if (other.isEmpty())
        return;
    if (other.d->needsDetach() || !std::is_nothrow_move_constructible_v<T>)
        return append(other);

    if (d->needsDetach() || other.size() > d.freeSpaceAtEnd()) {
        DataPointer detached(DataPointer::allocateGrow(d, other.size(), QArrayData::GrowsAtEnd));

        if (!d->needsDetach())
            detached->moveAppend(begin(), end());
        else
            detached->copyAppend(cbegin(), cend());
        detached->moveAppend(other.begin(), other.end());

        d.swap(detached);
    } else {
        // we're detached and we can just move data around
        d->moveAppend(other.begin(), other.end());
    }
}

template<typename T>
template<typename... Args>
inline typename QList<T>::reference QList<T>::emplaceFront(Args &&... args)
{
    if (d->needsDetach() || !d.freeSpaceAtBegin()) {
        DataPointer detached(DataPointer::allocateGrow(d, 1, QArrayData::GrowsAtBeginning));

        detached->emplaceBack(std::forward<Args>(args)...);
        if (!d.needsDetach())
            detached->moveAppend(d.begin(), d.end());
        else
            detached->copyAppend(constBegin(), constEnd());
        d.swap(detached);
    } else {
        d->emplaceFront(std::forward<Args>(args)...);
    }
    return *d.begin();
}


template <typename T>
inline typename QList<T>::iterator
QList<T>::insert(qsizetype i, qsizetype n, parameter_type t)
{
    Q_ASSERT_X(size_t(i) <= size_t(d->size), "QList<T>::insert", "index out of range");

    d->insert(i, n, t);
    return d.begin() + i;
}

template <typename T>
template <typename ...Args>
typename QList<T>::iterator
QList<T>::emplace(qsizetype i, Args&&... args)
{
    Q_ASSERT_X(i >= 0 && i <= d->size, "QList<T>::insert", "index out of range");

    if (d->needsDetach() || (d.size == d.constAllocatedCapacity())) {
        typename QArrayData::GrowthPosition pos = QArrayData::GrowsAtEnd;
        if (d.size != 0 && i <= (d.size >> 1))
            pos = QArrayData::GrowsAtBeginning;

        DataPointer detached(DataPointer::allocateGrow(d, 1, pos));
        const_iterator where = constBegin() + i;
        // Create an element here to handle cases when a user moves the element
        // from a container to the same container. This is a critical step for
        // COW types (e.g. Qt types) since copyAppend() done before emplace()
        // would shallow-copy the passed element and ruin the move
        T tmp(std::forward<Args>(args)...);

        detached->copyAppend(constBegin(), where);
        detached->emplace(detached.end(), std::move(tmp));
        detached->copyAppend(where, constEnd());
        d.swap(detached);
    } else {
        d->emplace(d.begin() + i, std::forward<Args>(args)...);
    }
    return d.begin() + i;
}

template<typename T>
template<typename... Args>
inline typename QList<T>::reference QList<T>::emplaceBack(Args &&... args)
{
    if (d->needsDetach() || !d.freeSpaceAtEnd()) {
        // condition below should follow the condition in QArrayDataPointer::reallocateGrow()
        if constexpr (!QTypeInfo<T>::isRelocatable || alignof(T) > alignof(std::max_align_t)) {
            // avoid taking a temporary copy of Args
            DataPointer detached(DataPointer::allocateGrow(d, 1, QArrayData::GrowsAtEnd));
            detached->copyAppend(constBegin(), constEnd());
            detached->emplace(detached.end(), std::forward<Args>(args)...);
            d.swap(detached);
        } else {
            // Create an element here to handle cases when a user moves the element
            // from a container to the same container. This is required as we call
            // reallocate, which could delete the data args points to.
            // This should be optimised to only take the copy when really required.
            T tmp(std::forward<Args>(args)...);
            DataPointer::reallocateGrow(d, 1);
            d->emplace(d.end(), std::move(tmp));
        }
    } else {
        d->emplaceBack(std::forward<Args>(args)...);
    }
    return *(d.end() - 1);
}

template <typename T>
typename QList<T>::iterator QList<T>::erase(const_iterator abegin, const_iterator aend)
{
    Q_ASSERT_X(isValidIterator(abegin), "QList::erase", "The specified iterator argument 'abegin' is invalid");
    Q_ASSERT_X(isValidIterator(aend), "QList::erase", "The specified iterator argument 'aend' is invalid");
    Q_ASSERT(aend >= abegin);

    qsizetype i = std::distance(d.constBegin(), abegin);
    qsizetype n = std::distance(abegin, aend);
    remove(i, n);

    return d.begin() + i;
}

template <typename T>
inline QList<T> &QList<T>::fill(parameter_type t, qsizetype newSize)
{
    if (newSize == -1)
        newSize = size();
    if (d->needsDetach() || newSize > capacity()) {
        // must allocate memory
        DataPointer detached(Data::allocate(d->detachCapacity(newSize)));
        detached->copyAppend(newSize, t);
        d.swap(detached);
    } else {
        // we're detached
        const T copy(t);
        d->assign(d.begin(), d.begin() + qMin(size(), newSize), t);
        if (newSize > size())
            d->copyAppend(newSize - size(), copy);
    }
    return *this;
}

namespace QtPrivate {
template <typename T, typename U>
qsizetype indexOf(const QList<T> &vector, const U &u, qsizetype from) noexcept
{
    if (from < 0)
        from = qMax(from + vector.size(), qsizetype(0));
    if (from < vector.size()) {
        auto n = vector.begin() + from - 1;
        auto e = vector.end();
        while (++n != e)
            if (*n == u)
                return qsizetype(n - vector.begin());
    }
    return -1;
}

template <typename T, typename U>
qsizetype lastIndexOf(const QList<T> &vector, const U &u, qsizetype from) noexcept
{
    if (from < 0)
        from += vector.d->size;
    else if (from >= vector.size())
        from = vector.size() - 1;
    if (from >= 0) {
        auto b = vector.begin();
        auto n = vector.begin() + from + 1;
        while (n != b) {
            if (*--n == u)
                return qsizetype(n - b);
        }
    }
    return -1;
}
}

template <typename T>
template <typename AT>
qsizetype QListSpecialMethodsBase<T>::indexOf(const AT &t, qsizetype from) const noexcept
{
    return QtPrivate::indexOf(*self(), t, from);
}

template <typename T>
template <typename AT>
qsizetype QListSpecialMethodsBase<T>::lastIndexOf(const AT &t, qsizetype from) const noexcept
{
    return QtPrivate::lastIndexOf(*self(), t, from);
}

template <typename T>
inline QList<T> QList<T>::mid(qsizetype pos, qsizetype len) const
{
    qsizetype p = pos;
    qsizetype l = len;
    using namespace QtPrivate;
    switch (QContainerImplHelper::mid(d.size, &p, &l)) {
    case QContainerImplHelper::Null:
    case QContainerImplHelper::Empty:
        return QList();
    case QContainerImplHelper::Full:
        return *this;
    case QContainerImplHelper::Subset:
        break;
    }

    // Allocate memory
    DataPointer copied(Data::allocate(l));
    copied->copyAppend(constBegin() + p, constBegin() + p + l);
    return copied;
}

Q_DECLARE_SEQUENTIAL_ITERATOR(List)
Q_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR(List)

template <typename T>
size_t qHash(const QList<T> &key, size_t seed = 0)
    noexcept(noexcept(qHashRange(key.cbegin(), key.cend(), seed)))
{
    return qHashRange(key.cbegin(), key.cend(), seed);
}

QList<uint> QStringView::toUcs4() const { return QtPrivate::convertToUcs4(*this); }

QT_END_NAMESPACE

#include <QtCore/qbytearraylist.h>
#include <QtCore/qstringlist.h>

#endif // QLIST_H
