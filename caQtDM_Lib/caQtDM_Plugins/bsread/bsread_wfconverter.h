#ifndef BSREAD_WFCONVERTER_H
#define BSREAD_WFCONVERTER_H

#include <QAtomicInt>
#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include <QElapsedTimer>

#include "knobData.h"
#include "mutexKnobData.h"
#include "bsread_channeldata.h"

template <class T_BSREAD,class T_CAQTDM>
class bsread_wfConverter
{
private:
    knobData* kDataP;
    bsread_channeldata * bsreadPVP;
public:
    bsread_wfConverter(knobData* kData,bsread_channeldata * bsreadPV)
    {
        kDataP=kData;
        bsreadPVP=bsreadPV;
    }

    void wfconvert()
    {

        QElapsedTimer timer;
        timer.start();


        if (kDataP->edata.valueCount!=bsreadPVP->bsdata.wf_data_size){
            QMutex *datamutex;
            datamutex = (QMutex*) kDataP->mutex;
            datamutex->lock();
            if (kDataP->edata.dataB==NULL){
                qDebug() << "Realloc";
                free(kDataP->edata.dataB);
            }

            kDataP->edata.dataB=malloc(bsreadPVP->bsdata.wf_data_size*sizeof(T_CAQTDM));
            kDataP->edata.dataSize=bsreadPVP->bsdata.wf_data_size*sizeof(T_CAQTDM);
            datamutex->unlock();
        }
        kDataP->edata.valueCount=bsreadPVP->bsdata.wf_data_size;

        QByteArray data = QByteArray::fromRawData((const char *)bsreadPVP->bsdata.wf_data, bsreadPVP->bsdata.wf_data_size*sizeof(T_BSREAD));
        QDataStream stream(data);
        //QVector<T_CAQTDM> data_stream(bsreadPVP->bsdata.wf_data_size);
        stream.setByteOrder(QDataStream::LittleEndian);
        ulong counter=0;


        while ( !stream.atEnd() ) {
             T_BSREAD datatemp;
             stream >> datatemp;

             ((T_CAQTDM*)kDataP->edata.dataB)[counter]=(T_CAQTDM)datatemp;
             counter++;
        }


        qDebug() << "convert timer :" <<  timer.elapsed() << "milliseconds";

      }

};






#endif // BSREAD_WFCONVERTER_H

