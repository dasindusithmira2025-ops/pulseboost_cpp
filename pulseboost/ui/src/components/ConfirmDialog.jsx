import { AlertTriangle, Shield } from "lucide-react";

import StatusBadge from "./StatusBadge";

export default function ConfirmDialog({
  open,
  tone = "warning",
  title,
  message,
  confirmLabel = "Confirm",
  cancelLabel = "Cancel",
  details = [],
  onConfirm,
  onCancel,
}) {
  if (!open) return null;

  const badge = tone === "danger" ? "WARNING" : tone === "accent" ? "ACTIVE" : "NEEDS_ATTENTION";
  const operationId = details?.[0]?.value || "PB-ACTION";

  return (
    <div className="dialog-backdrop" role="presentation" onClick={onCancel}>
      <div
        className={`dialog-card tone-${tone}`}
        role="dialog"
        aria-modal="true"
        aria-labelledby="pulseboost-dialog-title"
        onClick={(event) => event.stopPropagation()}
      >
        <div className="dialog-hero">
          <div className="dialog-hero-icon">
            {tone === "danger" ? <AlertTriangle size={22} /> : <Shield size={22} />}
          </div>
          <div className="dialog-hero-copy">
            <p className="eyebrow">Action gate</p>
            <h2 id="pulseboost-dialog-title" className="dialog-title">{title}</h2>
            <StatusBadge label={badge} />
          </div>
        </div>

        <div className="dialog-message-block">
          <p className="dialog-message">{message}</p>
        </div>

        {details.length ? (
          <div className="dialog-detail-card">
            <div className="detail-list">
              {details.map((detail) => (
                <div className="detail-row" key={detail.label}>
                  <span>{detail.label}</span>
                  <span className="mono">{detail.value}</span>
                </div>
              ))}
            </div>
          </div>
        ) : null}

        <div className="dialog-actions">
          <button className="button button-secondary" type="button" onClick={onCancel}>
            {cancelLabel}
          </button>
          <button className={`button ${tone === "danger" ? "button-danger" : "button-primary"}`} type="button" onClick={onConfirm}>
            {confirmLabel}
          </button>
        </div>

        <div className="dialog-footer-meta">
          <span>Operation</span>
          <strong>{operationId}</strong>
        </div>
      </div>
    </div>
  );
}
