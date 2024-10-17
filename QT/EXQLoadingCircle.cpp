#include "stdafx.h"
#include "EXQLoadingCircle.hpp"

#include <QPainter>

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

        void qLoadingCircle::Start()
        {
            if( _timer != nullptr )
            {
                _timer->start( _nUpdateMsec );
                connect( _timer, &QTimer::timeout, this, &qLoadingCircle::updateAnimation );
            }
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

            // 원의 중심과 반지름 설정
            int centerX = width() / 2;
            int centerY = height() / 2;
            int radius = 40;

            if( _isFirstRun == false )
            {
                int nPrevIdx = _nCurrentIdx - 1;
                if( nPrevIdx < 0 )
                    nPrevIdx = _vecColor.size() - 1;

                // 선 색상 및 두께 설정
                QPen pen( QBrush( _vecColor.at( nPrevIdx ) ), 5 );
                pen.setWidth( 5 );
                pen.setCapStyle( Qt::RoundCap );
                _painter->setPen( pen );
                _painter->drawArc( centerX - radius, centerY - radius, radius * 2, radius * 2, 90 * 16, -360 * 16 );
            }


            // 선 색상 및 두께 설정
            QPen pen( QBrush( _vecColor.at( _nCurrentIdx ) ), 5 );
            pen.setWidth( 5 );
            _painter->setPen( pen );

            // 원의 일부분만 그리기
            _painter->drawArc( centerX - radius, centerY - radius, radius * 2, radius * 2, 90 * 16, -_nCurrentLength * 16 + 1 );

            if( _nCurrentLength >= 360 )
                _painter->save();

            _painter->end();
        }

        void qLoadingCircle::updateAnimation()
        {
            if( _nCurrentLength < 360 )
            { // 360도까지 증가
                _nCurrentLength += 10; // 증가 속도 조정
                update(); // 다시 그리기 요청
            }
            else
            {
                _isFirstRun = false;

                if( ++_nCurrentIdx >= _vecColor.size() )
                    _nCurrentIdx = 0;

                _nCurrentLength = 0;
                update(); // 다시 그리기 요청
            }
        }

        void qLoadingCircle::init()
        {
            _timer = new QTimer( this );
            _painter = new QPainter( this );
            setAttribute( Qt::WA_TranslucentBackground );
        }
    }
}
