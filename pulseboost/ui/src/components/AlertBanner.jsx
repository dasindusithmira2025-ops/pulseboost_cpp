import { AlertTriangle, ShieldAlert } from "lucide-react";

import StatusBadge from "./StatusBadge";
import { severityTone } from "../utils/formatters";

export default function AlertBanner({ alert, onAction }) {
  const tone = severityTone(alert.severity);
  const resolvedTone = tone === "amber" ? "warning" : tone === "green" ? "success" : tone;
  const Icon = resolvedTone === "danger" ? ShieldAlert : AlertTriangle;

  return (
    <div className={`alert-banner tone-${resolvedTone}`}>
      <div className="alert-banner-main">
        <div className="alert-banner-icon">
          <Icon size={16} />
        </div>
        <div className="alert-banner-copy">
          <div className="badge-row">
            <StatusBadge label={alert.severity || "WARNING"} />
            {alert.metric ? <span className="pill-text">{alert.metric}</span> : null}
          </div>
          <strong>{alert.message}</strong>
        </div>
      </div>
      {alert.action ? (
        <button className="button button-secondary" type="button" onClick={() => onAction?.(alert)}>
          {alert.action}
        </button>
      ) : null}
    </div>
  );
}
