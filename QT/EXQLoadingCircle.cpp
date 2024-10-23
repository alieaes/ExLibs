#include "stdafx.h"
#include "EXQLoadingCircle.hpp"

#include <qcoreapplication.h>
#include <QPainter>
#include <QWindow>
#include <QThread>

namespace Ext
{
    namespace QT
    {
        QT::qLoadingCircle::qLoadingCircle( QWidget* parent, ::Qt::WindowFlags f )
            : QDialog( parent, f )
        {
            _size = QSize( 100, 100 );
            resize( _size );
            SetDefaultColor();

            init();
        }

        qLoadingCircle::~qLoadingCircle()
        {
        }

        void qLoadingCircle::Show()
        {
            Start();

            if( QThread::currentThread() == QCoreApplication::instance()->thread() )
                show();
            else
                QMetaObject::invokeMethod( this, "show" );
        }

        void qLoadingCircle::Exec()
        {
            Start();
            QMetaObject::invokeMethod( this, "exec" );
        }

        void qLoadingCircle::Start()
        {
            if( _timer != nullptr )
            {
                auto wdgTop = getTopLevelWidget();

                QWidget* wdgParent = parentWidget();
                int nX = 0;
                int nY = 0;

                while( wdgParent != wdgTop )
                {
                    nX += wdgParent->pos().x();
                    nY += wdgParent->pos().y();
                    wdgParent = wdgParent->parentWidget();

                    if( wdgParent == nullptr )
                        break;
                }

                auto parentRect = parentWidget()->rect();
                auto parentWindowGeometry = parentWidget()->window()->geometry();
                move( parentWindowGeometry.x() + nX + ( parentRect.width() / 2 ) - ( width() / 2 ), parentWindowGeometry.y() + nY + ( parentRect.height() / 2 ) - ( height() / 2 ) );

                _timer->start( _nUpdateMsec );
                connect( _timer, &QTimer::timeout, this, &qLoadingCircle::updateAnimation );
            }
        }

        void qLoadingCircle::Close()
        {
            QMetaObject::invokeMethod( this, "close" );
        }

        void qLoadingCircle::Stop()
        {
            if( _timer != nullptr )
            {
                delete _timer;
                _timer = nullptr;
            }

            _nCurrentIdx = 0;
            _nCurrentLength = 0;
            _isFirstRun = true;

            init();
        }

        void qLoadingCircle::SetSpeed( int nSpeed )
        {
            _nUpdateMsec = nSpeed;
        }

        void qLoadingCircle::AddColor( const QColor& color )
        {
        }

        void qLoadingCircle::AddGradientColor( const QGradient& gradient )
        {
            _vecColor.push_back( QBrush( gradient ) );
        }

        void qLoadingCircle::AddGradientAuto( const QColor& color1, const QColor& color2 )
        {
            int centerX = width() / 2;
            int centerY = height() / 2;

            QConicalGradient gradient;
            gradient.setCenter( centerX, centerY );
            gradient.setAngle( 93 );
            gradient.setColorAt( 0, color1 );
            gradient.setColorAt( 1, color2 );
            _vecColor.push_back( QBrush( gradient ) );
        }

        void qLoadingCircle::ClearColor()
        {
            _vecColor.clear();
        }

        void qLoadingCircle::SetDefaultColor()
        {
            _vecColor.clear();

            AddGradientAuto( "#3eccd5", "#6c8ad6" );
            AddGradientAuto( "#6c8ad6", "#8d39d7" );
            AddGradientAuto( "#8d39d7", "#3eccd5" );
        }

        void qLoadingCircle::SetRadius( int nRadius )
        {
            _nRadius = nRadius;
        }

        void qLoadingCircle::SetPenSize( int nPenSize )
        {
            _nPenSize = nPenSize;
        }

        void qLoadingCircle::showEvent( QShowEvent* event )
        {
            QDialog::showEvent( event );
        }

        void qLoadingCircle::paintEvent( QPaintEvent* event )
        {
            _painter->begin( this );
            _painter->restore();
            _painter->setRenderHint( QPainter::Antialiasing );

            int centerX = width() / 2;
            int centerY = height() / 2;
            int radius = 40;

            if( _isFirstRun == false )
            {
                int nPrevIdx = _nCurrentIdx - 1;
                if( nPrevIdx < 0 )
                    nPrevIdx = _vecColor.size() - 1;

                QPen pen( QBrush( _vecColor.at( nPrevIdx ) ), 5 );
                pen.setWidth( 5 );
                pen.setCapStyle( Qt::RoundCap );
                _painter->setPen( pen );
                _painter->drawArc( centerX - radius, centerY - radius, radius * 2, radius * 2, 90 * 16, -360 * 16 );
            }

            QPen pen( QBrush( _vecColor.at( _nCurrentIdx ) ), 5 );
            pen.setWidth( 5 );
            _painter->setPen( pen );

            _painter->drawArc( centerX - radius, centerY - radius, radius * 2, radius * 2, 90 * 16, -_nCurrentLength * 16 + 1 );

            if( _nCurrentLength >= 360 )
                _painter->save();

            _painter->end();
        }

        void qLoadingCircle::updateAnimation()
        {
            if( _nCurrentLength < 360 )
            {
                _nCurrentLength += 10; 
                update(); 
            }
            else
            {
                _isFirstRun = false;

                if( ++_nCurrentIdx >= _vecColor.size() )
                    _nCurrentIdx = 0;

                _nCurrentLength = 0;
                update();
            }
        }

        void qLoadingCircle::init()
        {
            _timer = new QTimer( this );
            _painter = new QPainter( this );
            setAttribute( Qt::WA_TranslucentBackground );
        }

        QWidget* qLoadingCircle::getTopLevelWidget()
        {
            QWidget* topWidget = this;

            while( topWidget->parentWidget() != nullptr )
                topWidget = topWidget->parentWidget();

            return topWidget;
        }
    }
}
