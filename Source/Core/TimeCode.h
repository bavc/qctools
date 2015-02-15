/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef TimeCodeH
#define TimeCodeH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <string>
//---------------------------------------------------------------------------

class TimeCode
{
public:
    //constructor/Destructor
    TimeCode ();
    TimeCode (int Hours, int Minutes, int Seconds, int Frames, int FramesPerSecond, bool DropFrame, bool MustUseSecondField=false, bool IsSecondField=false);
    TimeCode (int Frames, int FramesPerSecond, bool DropFrame, bool MustUseSecondField=false, bool IsSecondField_=false);

    //Operators
    TimeCode &operator ++()
    {
        PlusOne();
        return *this;
    }
    TimeCode operator ++(int)
    {
        PlusOne();
        return *this;
    }
    TimeCode &operator --()
    {
        MinusOne();
        return *this;
    }
    TimeCode operator --(int)
    {
        MinusOne();
        return *this;
    }
    bool operator== (const TimeCode &tc) const
    {
        return Hours                ==tc.Hours
            && Minutes              ==tc.Minutes
            && Seconds              ==tc.Seconds
            && Frames               ==tc.Frames
            && FramesPerSecond      ==tc.FramesPerSecond
            && DropFrame            ==tc.DropFrame
            && MustUseSecondField   ==tc.MustUseSecondField
            && IsSecondField        ==tc.IsSecondField;
    }
    bool operator!= (const TimeCode &tc) const
    {
        return Hours                !=tc.Hours
            || Minutes              !=tc.Minutes
            || Seconds              !=tc.Seconds
            || Frames               !=tc.Frames
            || FramesPerSecond      !=tc.FramesPerSecond
            || DropFrame            !=tc.DropFrame
            || MustUseSecondField   !=tc.MustUseSecondField
            || IsSecondField        !=tc.IsSecondField;
    }

    //Helpers
    bool IsValid() const
    {
        return FramesPerSecond?true:false;
    }
    void PlusOne();
    void MinusOne();
    std::string ToString();
    int ToFrames();

public:
    int Hours;
    int Minutes;
    int Seconds;
    int Frames;
    int FramesPerSecond;
    bool  DropFrame;
    bool  MustUseSecondField;
    bool  IsSecondField;
    bool  IsNegative;
};

#endif
