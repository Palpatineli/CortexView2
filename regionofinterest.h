#ifndef REGIONOFINTEREST_H
#define REGIONOFINTEREST_H

#include <QRect>
#include <QReadWriteLock>

#ifndef _MASTER_H
typedef unsigned short uns16;
#endif  // _MASTER_H
#ifndef _PVCAM_H
/************************* Class 3: Region Definition ************************/
typedef struct {
    uns16 s1;                     /* First pixel in the serial register */
    uns16 s2;                     /* Last pixel in the serial register */
    uns16 sbin;                   /* Serial binning for this region */
    uns16 p1;                     /* First row in the parallel register */
    uns16 p2;                     /* Last row in the parallel register */
    uns16 pbin;                   /* Parallel binning for this region */
} rgn_type;
#endif  // _PVCAM_H

class RegionOfInterest {
public:
    static RegionOfInterest *getRegion();

    void setROILimit(const QRect &value);
    void setROILimit(const int x, const int y, const int width, const int height);

    void setROI(const QRect &value);
    void setROI(const int x, const int y, const int width, const int height);
    void setROI(const rgn_type &roi_in);
    void setROI(const QPoint &top_left, const QPoint &bottom_right);
    QRect getROI();
    rgn_type getRGN();

    void setBins(const int x_in, const int y_in);
    void setBins(const QPoint &bins_in);
    QPoint getBins();

    QSize getSize();
    int length();
    void reset();

    bool lock();
    void unlock();
    bool isLocked();

private:
    RegionOfInterest();
    RegionOfInterest(RegionOfInterest const &) = delete;
    void operator=(RegionOfInterest const &) = delete;

    QRect roi_limit, roi;
    int x_bin, y_bin;

    bool is_locked;
    QReadWriteLock roi_lock;
    QReadWriteLock lock_lock;

    bool validateROI();
};

#endif // REGIONOFINTEREST_H
