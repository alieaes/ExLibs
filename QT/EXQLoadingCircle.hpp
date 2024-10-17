#ifndef __HDR_QT_LOADING_CIRCLE__
#define __HDR_QT_LOADING_CIRCLE__

#include <QDialog>
#include <QTimer>
#include <QColor>
#include <QWidget>
#include "qnamespace.h"
#include <qflags.h>

namespace Ext
{
    namespace QT
    {
        class qLoadingCircle : public QDialog
        {
            Q_OBJECT

        public:
            qLoadingCircle( QWidget* parent = nullptr, Qt::WindowFlags f = Qt::FramelessWindowHint | Qt::Dialog );
            virtual ~qLoadingCircle();

            void                                     Start();
            void                                     Stop();

            void                                     SetSpeed( int nSpeed );
            void                                     SetRadius( int nRadius );
            void                                     SetPenSize( int nPenSize );

            void                                     AddColor( const QColor& color );
            void                                     AddGradientColor( const QGradient& gradient );
            void                                     AddGradientAuto( const QColor& color1, const QColor& color2 );

            void                                     ClearColor();

            void                                     SetDefaultColor();

        protected:
            void                                     showEvent( QShowEvent* event ) override;
            void                                     paintEvent( QPaintEvent* event ) override;

        private slots:
            void                                     updateAnimation();

        private:
            void                                     init();

            QVector< QBrush >                        _vecColor;
            QVector< QGradient >                     _vecGradient;

            QTimer*                                  _timer             = nullptr;

            QSize                                    _size;

            int                                      _nCurrentLength    = 0;
            int                                      _nCurrentIdx       = 0;
            bool                                     _isFirstRun        = true;

            int                                      _nUpdateMsec       = 25;
            int                                      _nRadius           = 40;
            int                                      _nPenSize          = 5;

            QPainter*                                _painter;
        };
    }
}

#endif