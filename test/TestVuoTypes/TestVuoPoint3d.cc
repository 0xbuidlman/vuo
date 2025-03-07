/**
 * @file
 * TestVuoPoint3d implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <float.h>

extern "C" {
#include "TestVuoTypes.h"
#include "VuoPoint2d.h"
#include "VuoPoint3d.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoPoint3d);

/**
 * Tests the VuoPoint3d type.
 */
class TestVuoPoint3d : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		VuoHeap_init();
	}

	void testStringConversion_data()
	{
		QTest::addColumn<QString>("initializer");
		QTest::addColumn<VuoPoint3d>("value");

		{
			VuoPoint3d p;
			p.x = -0.999;
			p.y = 0.42;
			p.z = 0.22;
			QTest::newRow("different values") << "{\"x\":-0.999000,\"y\":0.420000,\"z\":0.220000}" << p;
		}
	}
	void testStringConversion()
	{
		QFETCH(QString, initializer);
		QFETCH(VuoPoint3d, value);

		VuoPoint3d p = VuoPoint3d_makeFromString(initializer.toUtf8().constData());

		QCOMPARE(p.x,value.x);
		QCOMPARE(p.y,value.y);
		QCOMPARE(p.z,value.z);

		QCOMPARE(VuoPoint3d_getString(p), initializer.toUtf8().constData());
	}

	void testMakeNonzero_data()
	{
		QTest::addColumn<VuoPoint3d>("value");
		QTest::addColumn<VuoPoint3d>("expectedValue");

		QTest::newRow("zero")             << VuoPoint3d_make(0,0,0)                                  << VuoPoint3d_make( .000001,  .000001,  .000001);
		QTest::newRow("one")              << VuoPoint3d_make(1,1,1)                                  << VuoPoint3d_make(1,1,1);
		QTest::newRow("positive epsilon") << VuoPoint3d_make( DBL_EPSILON, DBL_EPSILON, DBL_EPSILON) << VuoPoint3d_make( .000001,  .000001,  .000001);
		QTest::newRow("negative epsilon") << VuoPoint3d_make(-DBL_EPSILON,-DBL_EPSILON,-DBL_EPSILON) << VuoPoint3d_make(-.000001, -.000001, -.000001);
	}
	void testMakeNonzero()
	{
		QFETCH(VuoPoint3d, value);
		QFETCH(VuoPoint3d, expectedValue);

		VuoPoint3d actualPoint3d = VuoPoint3d_makeNonzero(value);
		QCOMPARE(actualPoint3d.x+10., expectedValue.x+10.);
		QCOMPARE(actualPoint3d.y+10., expectedValue.y+10.);
		QCOMPARE(actualPoint3d.z+10., expectedValue.z+10.);

		VuoPoint2d actualPoint2d = VuoPoint2d_makeNonzero(VuoPoint2d_make(value.x, value.y));
		QCOMPARE(actualPoint2d.x+10., expectedValue.x+10.);
		QCOMPARE(actualPoint2d.y+10., expectedValue.y+10.);

		VuoReal actualReal = VuoReal_makeNonzero(value.x);
		QCOMPARE(actualReal+10., expectedValue.x+10.);
	}
};

QTEST_APPLESS_MAIN(TestVuoPoint3d)

#include "TestVuoPoint3d.moc"

