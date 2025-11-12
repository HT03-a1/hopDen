import { useEffect, useState } from 'react';
import { Marker, Popup } from 'react-leaflet';
import { Icon } from 'leaflet';

interface BlinkingUserMarkerProps {
  entity: any;
  isCurrentUser: boolean;
  color: string;
  isSOS: boolean;
  createCustomIcon: (color: string, isSOS: boolean) => Icon;
  shouldBlink: boolean;
  onMarkerClick: (entity: any) => void;
}

export default function BlinkingUserMarker({
  entity,
  isCurrentUser,
  color,
  isSOS,
  createCustomIcon,
  shouldBlink,
  onMarkerClick,
}: BlinkingUserMarkerProps) {
  const [isBlinking, setIsBlinking] = useState(false);

  // Blinking effect for current user's GPS when SOS modal is open
  useEffect(() => {
    if (shouldBlink) {
      const interval = setInterval(() => {
        setIsBlinking(prev => !prev);
      }, 500); // Blink every 500ms
      return () => clearInterval(interval);
    } else {
      setIsBlinking(false);
    }
  }, [shouldBlink]);

  const icon = isBlinking
    ? createCustomIcon('yellow', false) // Use yellow when blinking
    : createCustomIcon(color, isSOS);

  return (
    <Marker
      key={`${entity.type}-${entity.id}`}
      position={[entity.lat, entity.lon]}
      icon={icon}
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
          {entity.type === 'sos' && (
            <div className="mt-2">
              <p className="text-xs">Mức độ: {entity.severity}</p>
              <p className="text-xs">Trạng thái: {entity.status}</p>
            </div>
          )}
        </div>
      </Popup>
    </Marker>
  );
}

