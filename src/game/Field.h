#ifndef HEADER_FIELD_H
#define HEADER_FIELD_H

class Cube;

#include "NoCopy.h"
#include "V2.h"

class Field : public NoCopy {
    private:
        int m_w;
        int m_h;
        Cube ***m_marks;
        Cube *m_border;
    public:
        Field(int w, int h);
        ~Field();

        int getW() const { return m_w; }
        int getH() const { return m_h; }

        Cube *getModel(const V2 &loc);
        void setModel(const V2 &loc, Cube *model);
};

#endif
