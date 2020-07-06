/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <private/qflatmap_p.h>
#include <qbytearray.h>
#include <qstring.h>
#include <qstringview.h>
#include <qvarlengtharray.h>

#include <algorithm>
#include <list>
#include <tuple>

class tst_QFlatMap : public QObject
{
    Q_OBJECT
private slots:
    void constructing();
    void constAccess();
    void insertion();
    void removal();
    void extraction();
    void iterators();
    void statefulComparator();
    void transparency();
    void viewIterators();
    void varLengthArray();
};

void tst_QFlatMap::constructing()
{
    using Map = QFlatMap<int, QByteArray>;
    Map fmDefault;
    QVERIFY(fmDefault.isEmpty());
    QCOMPARE(fmDefault.size(), Map::size_type(0));
    QCOMPARE(fmDefault.size(), fmDefault.count());

    auto key_compare = fmDefault.key_comp();
    auto selfbuilt_value_compare
            = [&key_compare](const Map::value_type &a, const Map::value_type &b)
              {
                  return key_compare(a.first, b.first);
              };
    auto value_compare = fmDefault.value_comp();

    Map::key_container_type kv = { 6, 2, 1 };
    Map::mapped_container_type mv = { "foo", "bar", "baz" };
    Map fmCopy{kv, mv};
    QCOMPARE(fmCopy.size(), Map::size_type(3));
    QVERIFY(std::is_sorted(fmCopy.begin(), fmCopy.end(), selfbuilt_value_compare));
    QVERIFY(std::is_sorted(fmCopy.begin(), fmCopy.end(), value_compare));

    Map fmMove{
        Map::key_container_type{ 6, 2, 1 },
        Map::mapped_container_type{ "foo", "bar", "baz" }
    };
    QCOMPARE(fmMove.size(), Map::size_type(3));
    QVERIFY(std::is_sorted(fmMove.begin(), fmMove.end(), value_compare));

    auto fmInitList = Map{ { 1, 2 }, { "foo", "bar" } };
    QVERIFY(std::is_sorted(fmInitList.begin(), fmInitList.end(), value_compare));

    auto fmRange = Map(fmCopy.begin(), fmCopy.end());
    QVERIFY(std::is_sorted(fmRange.begin(), fmRange.end(), value_compare));

    kv.clear();
    mv.clear();
    std::vector<Map::value_type> sv;
    for (auto it = fmRange.begin(); it != fmRange.end(); ++it) {
        kv.push_back(it->first);
        mv.push_back(it->second);
        sv.push_back(*it);
    }
    auto fmFromSortedVectorCopy = Map(Qt::OrderedUniqueRange, kv, mv);
    auto fmFromSortedVectorMove = Map(Qt::OrderedUniqueRange, Map::key_container_type(kv),
                                      Map::mapped_container_type(mv));
    auto fmFromSortedInitList = Map(Qt::OrderedUniqueRange, { { 1, "foo" }, { 2, "bar" } });
    auto fmFromSortedRange = Map(Qt::OrderedUniqueRange, sv.begin(), sv.end());
}

void tst_QFlatMap::constAccess()
{
    using Map = QFlatMap<QByteArray, QByteArray>;
    const Map m{ { { "foo", "FOO" }, { "bar", "BAR" } } };

    const std::vector<Map::value_type> v{ { "foo", "FOO" }, { "bar", "BAR" } };

    QCOMPARE(m.value("foo").data(), "FOO");
    QCOMPARE(m.value("bar").data(), "BAR");
    QCOMPARE(m.value("nix"), QByteArray());
    QCOMPARE(m.value("nix", "NIX").data(), "NIX");
    QCOMPARE(m["foo"].data(), "FOO");
    QCOMPARE(m["bar"].data(), "BAR");
    QCOMPARE(m["nix"], QByteArray());
    QVERIFY(m.contains("foo"));
    QVERIFY(!m.contains("nix"));
}

void tst_QFlatMap::insertion()
{
    using Map = QFlatMap<QByteArray, QByteArray>;
    Map m;
    QByteArray foo = "foo";
    m[foo] = foo.toUpper();
    m["bar"] = "BAR";
    m["baz"] = "BAZ";
    QVERIFY(m.insert("oof", "eek").second);
    QVERIFY(!m.insert("oof", "OOF").second);
    const std::vector<Map::value_type> container = { { "bla", "BLA" }, { "blubb", "BLUBB" } };
    m.insert(container.begin(), container.end());
    QCOMPARE(m.value("foo").data(), "FOO");
    QCOMPARE(m.value("bar").data(), "BAR");
    QCOMPARE(m.value("baz").data(), "BAZ");
    QCOMPARE(m.value("oof").data(), "OOF");
    QCOMPARE(m.value("bla").data(), "BLA");
    QCOMPARE(m.value("blubb").data(), "BLUBB");

    Map::value_type a1[] = { { "narf", "NARF" },
                             { "zort", "ZORT" },
                             { "troz", "TROZ" } };
    Map::value_type a2[] = { { "gnampf", "GNAMPF" },
                             { "narf", "NARFFFF" },
                             { "narf", "NARFFFFF" },
                             { "narf", "NARFFFFFF" } };
    m.insert(std::begin(a1), std::end(a1));
    m.insert(Qt::OrderedUniqueRange, std::begin(a2), std::end(a2));
    QCOMPARE(m.size(), 10);
    QCOMPARE(m.value("narf").data(), "NARFFFFFF");
    QCOMPARE(m.value("gnampf").data(), "GNAMPF");
}

void tst_QFlatMap::extraction()
{
    using Map = QFlatMap<int, QByteArray>;
    Map::key_container_type expectedKeys = { 1, 2, 3 };
    Map::mapped_container_type expectedValues = { "een", "twee", "dree" };
    Map m(expectedKeys, expectedValues);
    auto keys = m.keys();
    auto values = m.values();
    QCOMPARE(keys, expectedKeys);
    QCOMPARE(values, expectedValues);
    Map::containers c = std::move(m).extract();
    QCOMPARE(c.keys, expectedKeys);
    QCOMPARE(c.values, expectedValues);
}

void tst_QFlatMap::iterators()
{
    using Map = QFlatMap<int, QByteArray>;
    auto m = Map{ { 1, "foo" }, { 2, "bar" }, { 3, "baz" } };
    {
        // forward / backward
        Map::iterator a = m.begin();
        QVERIFY(a != m.end());
        QCOMPARE(a.key(), 1);
        QCOMPARE(a.value(), "foo");
        ++a;
        QCOMPARE(a.key(), 2);
        QCOMPARE(a.value(), "bar");
        Map::iterator b = a++;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(3, "baz"));
        QCOMPARE(std::tie(b.key(), b.value()), std::make_tuple(2, "bar"));
        QCOMPARE(++a, m.end());
        --a;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(3, "baz"));
        a.value() = "buzz";
        b = a--;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(2, "bar"));
        QCOMPARE(std::tie(b.key(), b.value()), std::make_tuple(3, "buzz"));
        b.value() = "baz";

        // random access
        a = m.begin();
        a += 2;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(3, "baz"));
        a = m.begin() + 1;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(2, "bar"));
        a = 1 + m.begin();
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(2, "bar"));
        a = m.end() - 1;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(3, "baz"));
        b = m.end();
        b -= 1;
        QCOMPARE(std::tie(b.key(), b.value()), std::make_tuple(3, "baz"));
        QCOMPARE(m.end() - m.begin(), m.size());

        // comparison
        a = m.begin() + m.size() - 1;
        b = m.end() - 1;
        QVERIFY(a == b);
        a = m.begin();
        b = m.end();
        QVERIFY(a < b);
        QVERIFY(a <= b);
        QVERIFY(b > a);
        QVERIFY(b >= a);
        a = b;
        QVERIFY(!(a < b));
        QVERIFY(a <= b);
        QVERIFY(!(b > a));
        QVERIFY(b >= a);

        // de-referencing
        a = m.begin();
        auto ref0 = *a;
        QCOMPARE(ref0.first, 1);
        QCOMPARE(ref0.second, "foo");
        auto ref1 = a[1];
        QCOMPARE(ref1.first, 2);
        QCOMPARE(ref1.second, "bar");
    }
    {
        // forward / backward
        Map::const_iterator a = m.cbegin();
        QVERIFY(a != m.cend());
        QCOMPARE(a.key(), 1);
        QCOMPARE(a.value(), "foo");
        ++a;
        QCOMPARE(a.key(), 2);
        QCOMPARE(a.value(), "bar");
        Map::const_iterator b = a++;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(3, "baz"));
        QCOMPARE(std::tie(b.key(), b.value()), std::make_tuple(2, "bar"));
        QCOMPARE(++a, m.cend());
        --a;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(3, "baz"));
        b = a--;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(2, "bar"));

        // random access
        a = m.cbegin();
        a += 2;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(3, "baz"));
        a = m.cbegin() + 1;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(2, "bar"));
        a = 1 + m.cbegin();
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(2, "bar"));
        a = m.cend() - 1;
        QCOMPARE(std::tie(a.key(), a.value()), std::make_tuple(3, "baz"));
        b = m.cend();
        b -= 1;
        QCOMPARE(std::tie(b.key(), b.value()), std::make_tuple(3, "baz"));
        QCOMPARE(m.cend() - m.cbegin(), m.size());

        // comparison
        a = m.cbegin() + m.size() - 1;
        b = m.cend() - 1;
        QVERIFY(a == b);
        a = m.cbegin();
        b = m.cend();
        QVERIFY(a < b);
        QVERIFY(a <= b);
        QVERIFY(b > a);
        QVERIFY(b >= a);
        a = b;
        QVERIFY(!(a < b));
        QVERIFY(a <= b);
        QVERIFY(!(b > a));
        QVERIFY(b >= a);

        // de-referencing
        a = m.cbegin();
        auto ref0 = *a;
        QCOMPARE(ref0.first, 1);
        QCOMPARE(ref0.second, "foo");
        auto ref1 = a[1];
        QCOMPARE(ref1.first, 2);
        QCOMPARE(ref1.second, "bar");
    }
    {
        Map::iterator it = m.begin();
        Map::const_iterator cit = it;
        Q_UNUSED(it);
        Q_UNUSED(cit);
    }
    {
        std::list<Map::value_type> revlst;
        std::copy(m.begin(), m.end(), std::front_inserter(revlst));
        std::vector<Map::value_type> v0;
        std::copy(revlst.begin(), revlst.end(), std::back_inserter(v0));
        std::vector<Map::value_type> v1;
        std::copy(m.rbegin(), m.rend(), std::back_inserter(v1));
        const Map cm = m;
        std::vector<Map::value_type> v2;
        std::copy(cm.rbegin(), cm.rend(), std::back_inserter(v2));
        std::vector<Map::value_type> v3;
        std::copy(m.crbegin(), m.crend(), std::back_inserter(v3));
        QCOMPARE(v0, v1);
        QCOMPARE(v1, v2);
        QCOMPARE(v2, v3);
    }
}

void tst_QFlatMap::removal()
{
    using Map = QFlatMap<int, QByteArray>;
    Map m({ { 2, "bar" }, { 3, "baz" }, { 1, "foo" } });
    QCOMPARE(m.value(2).data(), "bar");
    QCOMPARE(m.take(2).data(), "bar");
    QVERIFY(!m.contains(2));
    QCOMPARE(m.size(), Map::size_type(2));
    QVERIFY(m.remove(1));
    QVERIFY(!m.contains(1));
    QVERIFY(!m.remove(1));
    QCOMPARE(m.size(), Map::size_type(1));
    m.clear();
    QVERIFY(m.isEmpty());
    QVERIFY(m.empty());

    m[1] = "een";
    m[2] = "twee";
    m[3] = "dree";
    auto it = m.lower_bound(1);
    QCOMPARE(it.key(), 1);
    it = m.erase(it);
    QCOMPARE(it.key(), 2);
    QVERIFY(!m.contains(1));
}

void tst_QFlatMap::statefulComparator()
{
    struct CountingCompare {
        mutable int count = 0;

        bool operator()(const QString &lhs, const QString &rhs) const
        {
            ++count;
            return lhs < rhs;
        }
    };

    using Map = QFlatMap<QString, QString, CountingCompare>;
    auto m1 = Map{ { "en", "een"}, { "to", "twee" }, { "tre", "dree" } };
    QVERIFY(m1.key_comp().count > 0);
    auto m2 = Map(m1.key_comp());
    QCOMPARE(m2.key_comp().count, m1.key_comp().count);
    m2.insert(m1.begin(), m1.end());
    QVERIFY(m2.key_comp().count > m1.key_comp().count);
}

void tst_QFlatMap::transparency()
{
    struct StringViewCompare
    {
        using is_transparent = void;
        bool operator()(const QStringView &lhs, const QStringView &rhs) const
        {
            return lhs < rhs;
        }
    };

    using Map = QFlatMap<QString, QString, StringViewCompare>;
    auto m = Map{ { "one", "een" }, { "two", "twee" }, { "three", "dree" } };

    const QString numbers = "one two three";
    const QStringView sv1{numbers.constData(), 3};
    const QStringView sv2{numbers.constData() + 4, 3};
    const QStringView sv3{numbers.constData() + 8, 5};
    QCOMPARE(m.lower_bound(sv1).value(), "een");
    QCOMPARE(m.lower_bound(sv2).value(), "twee");
    QCOMPARE(m.lower_bound(sv3).value(), "dree");
}

void tst_QFlatMap::viewIterators()
{
    using Map = QFlatMap<QByteArray, QByteArray>;
    Map m({ { "yksi", "een"}, { "kaksi", "twee" }, { "kolme", "dree" } });
    {
        std::vector<QByteArray> keys;
        std::transform(m.begin(), m.end(), std::back_inserter(keys),
                       [](const Map::value_type &v)
                       {
                           return v.first;
                       });
        auto it = keys.begin();
        QCOMPARE(*it, "kaksi");
        QCOMPARE(it->length(), 5);
        ++it;
        QCOMPARE(*it, "kolme");
        it++;
        QCOMPARE(*it, "yksi");
        ++it;
        QCOMPARE(it, keys.end());
        --it;
        QCOMPARE(*it, "yksi");
        it--;
        QCOMPARE(*it, "kolme");
    }
    {
        std::vector<QByteArray> values;
        std::transform(m.begin(), m.end(), std::back_inserter(values),
                       [](const Map::value_type &v)
                       {
                           return v.second;
                       });
        auto it = values.begin();
        QCOMPARE(*it, "twee");
        QCOMPARE(it->length(), 4);
        ++it;
        QCOMPARE(*it, "dree");
        it++;
        QCOMPARE(*it, "een");
        ++it;
        QCOMPARE(it, values.end());
        --it;
        QCOMPARE(*it, "een");
        it--;
        QCOMPARE(*it, "dree");
    }
}

void tst_QFlatMap::varLengthArray()
{
    using Map = QFlatMap<int, QByteArray, std::less<int>,
                         QVarLengthArray<int, 1024>, QVarLengthArray<QByteArray, 1024>>;
    Map m{ { 2, "twee" } };
    m.insert(1, "een");
    m.remove(1);
    QVERIFY(!m.isEmpty());
    m.remove(2);
    QVERIFY(m.isEmpty());
}

QTEST_APPLESS_MAIN(tst_QFlatMap)
#include "tst_qflatmap.moc"
