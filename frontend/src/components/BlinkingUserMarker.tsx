import { useEffect, useState, useRef } from 'react';
import { Marker, Popup } from 'react-leaflet';
import { Icon, Marker as LeafletMarker } from 'leaflet';

interface BlinkingUserMarkerProps {
  entity: any;
  color: string;
  isSOS: boolean;
  isLarge?: boolean;
  createCustomIcon: (color: string, isSOS: boolean, isLarge?: boolean) => Icon;
  shouldBlink: boolean;
  hasActiveSOS?: boolean;
  onMarkerClick: (entity: any) => void;
}

function MarkerWithBlink({
  entity,
  color,
  isSOS,
  isLarge,
  createCustomIcon,
  shouldBlink,
  hasActiveSOS,
  onMarkerClick,
}: BlinkingUserMarkerProps) {
  const markerRef = useRef<LeafletMarker | null>(null);
  const [isBlinking, setIsBlinking] = useState(false);

  // Blinking effect - SOS markers luôn nhấp nháy đỏ
  useEffect(() => {
    if (shouldBlink && isSOS) {
      // Fast blinking red for SOS markers - blink every 400ms
      const interval = setInterval(() => {
        setIsBlinking(prev => !prev);
      }, 400);
      return () => clearInterval(interval);
    } else if (hasActiveSOS) {
      // Fast blinking red when user has active SOS - blink every 400ms
      const interval = setInterval(() => {
        setIsBlinking(prev => !prev);
      }, 400);
      return () => clearInterval(interval);
    } else if (shouldBlink) {
      // Original blinking for modal
      const interval = setInterval(() => {
        setIsBlinking(prev => !prev);
      }, 500);
      return () => clearInterval(interval);
    } else {
      setIsBlinking(false);
    }
  }, [shouldBlink, hasActiveSOS, isSOS]);

  // Update icon and opacity when blinking state changes
  useEffect(() => {
    if (markerRef.current) {
      // SOS markers: luôn màu đỏ, to
      let newIcon;
      if (isSOS) {
        newIcon = createCustomIcon('red', false, true);
      } else if (hasActiveSOS) {
        newIcon = createCustomIcon('red', false, true);
      } else {
        newIcon = createCustomIcon(color, isSOS, isLarge);
      }
      markerRef.current.setIcon(newIcon);
      
      // Thay đổi opacity để tạo hiệu ứng nhấp nháy
      const element = markerRef.current.getElement();
      if (element) {
        if ((isSOS && isBlinking) || (hasActiveSOS && isBlinking)) {
          element.style.opacity = '0.5'; // Mờ khi blink
        } else if (isSOS || hasActiveSOS) {
          element.style.opacity = '1'; // Sáng khi không blink
        } else {
          element.style.opacity = '1';
        }
      }
    }
  }, [isBlinking, hasActiveSOS, color, isSOS, isLarge, createCustomIcon]);

  // Initial icon - SOS markers luôn màu đỏ và to
  const initialIcon = isSOS 
    ? createCustomIcon('red', false, true) 
    : createCustomIcon(color, isSOS, isLarge);

  // Log để debug SOS marker render (chỉ trong dev mode)
  if (import.meta.env.DEV && isSOS) {
    console.log('[BlinkingUserMarker] Rendering SOS marker:', {
      id: entity.id,
      position: [entity.lat, entity.lon],
      isSOS,
      isLarge,
      shouldBlink,
      hasActiveSOS
    });
  }

  return (
    <Marker
      ref={markerRef}
      position={[entity.lat, entity.lon]}
      icon={initialIcon}
      eventHandlers={{
        click: (e) => {
          e.originalEvent.stopPropagation();
          onMarkerClick(entity);
        },
      }}
    >
      <Popup>
        <div className="p-2">
          <h3 className="font-bold">{entity.name}</h3>
          <p className="text-sm text-gray-600">{entity.type}</p>
          {hasActiveSOS && (
            <div className="mt-2 p-2 bg-red-100 border-2 border-red-500 rounded">
              <p className="text-xs font-bold text-red-700">🚨 ĐANG CÓ SOS ACTIVE</p>
            </div>
          )}
          {entity.type === 'sos' && (
            <div className="mt-2 space-y-1">
              <div className="border-t pt-2">
                <p className="text-xs font-semibold text-gray-700">Thông tin người dùng:</p>
                <p className="text-xs">👤 Tên: {entity.userName || 'Không xác định'}</p>
                <p className="text-xs">📞 SĐT: {entity.userPhone || 'N/A'}</p>
                <p className="text-xs">📍 Định vị: {entity.lat?.toFixed(6) || 'N/A'}, {entity.lon?.toFixed(6) || 'N/A'}</p>
              </div>
              <div className="border-t pt-2 mt-2">
                <p className="text-xs font-semibold text-gray-700">Thông tin SOS:</p>
                <p className="text-xs">⚠️ Loại: {entity.sosType === 'accident' && '🚗 Tai nạn'}
                  {entity.sosType === 'breakdown' && '🔧 Hỏng xe'}
                  {entity.sosType === 'medical' && '🏥 Y tế'}
                  {entity.sosType === 'other' && '⚠️ Khác'}</p>
                <p className="text-xs">📊 Mức độ: {entity.severity}</p>
                <p className="text-xs">📋 Trạng thái: {entity.status === 'pending' && 'Chờ xử lý'}
                  {entity.status === 'accepted' && 'Đã nhận'}
                  {entity.status === 'on_route' && 'Đang đi'}
                  {entity.status === 'done' && 'Hoàn thành'}
                  {entity.status === 'cancelled' && 'Đã hủy'}</p>
                {entity.note && (
                  <p className="text-xs mt-1 text-gray-600">💬 Ghi chú: {entity.note}</p>
                )}
                {entity.createdAt && (
                  <p className="text-xs text-gray-500 mt-1">
                    🕐 {new Date(entity.createdAt).toLocaleString('vi-VN')}
                  </p>
                )}
              </div>
            </div>
          )}
        </div>
      </Popup>
    </Marker>
  );
}

export default function BlinkingUserMarker({
  entity,
  color,
  isSOS,
  isLarge = false,
  createCustomIcon,
  shouldBlink,
  hasActiveSOS = false,
  onMarkerClick,
}: BlinkingUserMarkerProps) {
  return (
    <MarkerWithBlink
      entity={entity}
      color={color}
      isSOS={isSOS}
      isLarge={isLarge}
      createCustomIcon={createCustomIcon}
      shouldBlink={shouldBlink}
      hasActiveSOS={hasActiveSOS}
      onMarkerClick={onMarkerClick}
    />
  );
}