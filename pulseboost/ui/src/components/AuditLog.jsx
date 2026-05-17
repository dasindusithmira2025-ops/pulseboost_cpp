import StatusBadge from "./StatusBadge";
import { formatClock } from "../utils/formatters";

export default function AuditLog({ actions, onExport }) {
  return (
    <section className="surface-card">
      <div className="surface-head">
        <div className="surface-copy">
          <p className="eyebrow">Runtime trail</p>
          <h2 className="surface-title">Operator and adaptive actions</h2>
          <p className="surface-description">Session-linked actions remain visible even when they are not primary audit entries.</p>
        </div>
        <div className="surface-actions">
          <button className="button button-secondary" type="button" onClick={onExport} disabled={!onExport}>
            Export CSV
          </button>
        </div>
      </div>
      <div className="timeline-stack">
        {actions.length ? actions.map((action, index) => (
          <article className="timeline-card" key={`${action.action_type}-${index}`}>
            <div className="timeline-head">
              <strong>{action.action_type}</strong>
              <StatusBadge label={action.score_delta < 0 ? "HELPED" : "LIVE"} subtle />
            </div>
            <span className="timeline-meta">{formatClock(action.timestamp)}</span>
            <p className="text-muted">{action.trigger_reason || "No trigger reasoning stored."}</p>
            <p className="timeline-detail">{action.ai_reasoning || "No advisory narrative stored."}</p>
          </article>
        )) : <div className="text-muted">No runtime actions logged yet.</div>}
      </div>
    </section>
  );
}
