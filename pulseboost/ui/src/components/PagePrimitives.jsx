import { ChevronRight, LoaderCircle, Lock, Sparkles, TriangleAlert, X } from "lucide-react";

import StatusBadge from "./StatusBadge";

export function PageHero({ eyebrow, title, description, badges, actions, stats, children, aside }) {
  return (
    <section className="hero-grid">
      <div className="hero-panel hero-panel-primary">
        <div className="hero-header">
          <div className="hero-copy">
            {eyebrow ? <p className="eyebrow">{eyebrow}</p> : null}
            <h1 className="hero-title">{title}</h1>
            {description ? <p className="hero-description">{description}</p> : null}
            {badges ? <div className="badge-row">{badges}</div> : null}
          </div>
          {actions ? <div className="hero-actions">{actions}</div> : null}
        </div>
        {stats?.length ? (
          <div className="hero-stats">
            {stats.map((item) => (
              <div className="hero-stat" key={item.label}>
                <span className="hero-stat-label">{item.label}</span>
                <strong className={`hero-stat-value${item.mono ? " mono" : ""}`}>{item.value}</strong>
                {item.detail ? <span className="hero-stat-detail">{item.detail}</span> : null}
              </div>
            ))}
          </div>
        ) : null}
        {children ? <div className="hero-body">{children}</div> : null}
      </div>
      {aside ? <div className="hero-panel hero-panel-secondary">{aside}</div> : null}
    </section>
  );
}

export function Surface({ title, eyebrow, description, actions, children, className = "" }) {
  return (
    <section className={`surface-card ${className}`.trim()}>
      {title || eyebrow || description || actions ? (
        <div className="surface-head">
          <div className="surface-copy">
            {eyebrow ? <p className="eyebrow">{eyebrow}</p> : null}
            {title ? <h2 className="surface-title">{title}</h2> : null}
            {description ? <p className="surface-description">{description}</p> : null}
          </div>
          {actions ? <div className="surface-actions">{actions}</div> : null}
        </div>
      ) : null}
      {children}
    </section>
  );
}

export function SectionHeader({ eyebrow, title, description, actions }) {
  return (
    <div className="section-header">
      <div className="section-copy">
        {eyebrow ? <p className="eyebrow">{eyebrow}</p> : null}
        <h2 className="section-title">{title}</h2>
        {description ? <p className="section-description">{description}</p> : null}
      </div>
      {actions ? <div className="section-actions">{actions}</div> : null}
    </div>
  );
}

export function SplitSurface({ left, right, rightClassName = "" }) {
  return (
    <div className="split-layout">
      <div className="split-column">{left}</div>
      <div className={`split-column ${rightClassName}`.trim()}>{right}</div>
    </div>
  );
}

export function StatGrid({ items, columns = 4 }) {
  return (
    <div className={`metric-grid metric-grid-${columns}`}>
      {items.map((item) => (
        <article className={`metric-card tone-${item.tone || "muted"}`} key={item.label}>
          <div className="metric-card-head">
            <span className="metric-card-label">{item.label}</span>
            {item.badge ? <div>{item.badge}</div> : null}
          </div>
          <strong className={`metric-card-value${item.mono ? " mono" : ""}`}>{item.value}</strong>
          {item.detail ? <p className="metric-card-detail">{item.detail}</p> : null}
          {item.progress !== undefined ? (
            <div className="progress-track" aria-hidden="true">
              <span className="progress-fill" style={{ width: `${Math.max(0, Math.min(100, Number(item.progress)))}%` }} />
            </div>
          ) : null}
        </article>
      ))}
    </div>
  );
}

export function Callout({ tone = "muted", title, detail, actions, icon = null }) {
  return (
    <div className={`callout-card tone-${tone}`}>
      <div className="callout-main">
        <div className="callout-icon">{icon || <Sparkles size={15} />}</div>
        <div className="callout-copy">
          <strong>{title}</strong>
          {detail ? <p className="text-muted">{detail}</p> : null}
        </div>
      </div>
      {actions ? <div className="callout-actions">{actions}</div> : null}
    </div>
  );
}

export function EmptyState({ label, detail, icon = null, action = null }) {
  return (
    <div className="state-card">
      <div className="state-icon">{icon || <TriangleAlert size={18} />}</div>
      <div className="state-copy">
        <strong>{label}</strong>
        {detail ? <p className="text-muted">{detail}</p> : null}
      </div>
      {action}
    </div>
  );
}

export function LoadingState({ label }) {
  return (
    <div className="state-card">
      <div className="state-icon">
        <LoaderCircle size={18} className="spin" />
      </div>
      <div className="state-copy">
        <strong>{label}</strong>
      </div>
    </div>
  );
}

export function UnsupportedState({ title, detail, badge = "UNSUPPORTED", action = null }) {
  return (
    <div className="unsupported-card">
      <div className="unsupported-head">
        <StatusBadge label={badge} />
        <strong>{title}</strong>
      </div>
      <p className="text-muted">{detail}</p>
      {action}
    </div>
  );
}

export function InlineTabs({ options, value, onChange }) {
  return (
    <div className="segmented-control" role="tablist">
      {options.map((option) => (
        <button
          key={option.value}
          className={`segment-button ${value === option.value ? "active" : ""}`}
          type="button"
          role="tab"
          aria-selected={value === option.value}
          onClick={() => onChange(option.value)}
        >
          {option.label}
        </button>
      ))}
    </div>
  );
}

export function Field({ label, hint, children }) {
  return (
    <label className="field">
      <span className="field-label">{label}</span>
      {children}
      {hint ? <span className="field-hint">{hint}</span> : null}
    </label>
  );
}

export function DetailList({ items }) {
  if (!items?.length) {
    return <div className="text-muted">No structured detail available.</div>;
  }

  return (
    <div className="detail-list">
      {items.map((item) => (
        <div className="detail-row" key={item.label}>
          <span>{item.label}</span>
          <span className={item.mono ? "mono" : ""}>{item.value}</span>
        </div>
      ))}
    </div>
  );
}

export function TableCard({
  columns,
  rows,
  emptyLabel = "No rows available.",
  onRowClick,
  selectedKey,
  dense = false,
}) {
  return (
    <div className={`table-card${dense ? " dense" : ""}`}>
      <table className="data-table">
        <thead>
          <tr>
            {columns.map((column) => <th key={column.key}>{column.label}</th>)}
          </tr>
        </thead>
        <tbody>
          {rows.length ? rows.map((row) => (
            <tr
              key={row.key}
              className={selectedKey === row.key ? "selected" : ""}
              onClick={onRowClick ? () => onRowClick(row.key) : undefined}
            >
              {columns.map((column) => <td key={`${row.key}-${column.key}`}>{column.render(row)}</td>)}
            </tr>
          )) : (
            <tr>
              <td colSpan={columns.length} className="table-empty">{emptyLabel}</td>
            </tr>
          )}
        </tbody>
      </table>
    </div>
  );
}

export function FeatureLock({ title, detail, badge = "LOCKED", actionLabel, onAction }) {
  return (
    <div className="lock-card">
      <div className="lock-head">
        <div className="lock-title">
          <Lock size={14} />
          <strong>{title}</strong>
        </div>
        <StatusBadge label={badge} />
      </div>
      <p className="text-muted">{detail}</p>
      {actionLabel ? (
        <button className="button button-secondary" type="button" onClick={onAction}>
          {actionLabel}
        </button>
      ) : null}
    </div>
  );
}

export function SelectorList({ items, selectedKey, onSelect, renderMeta }) {
  return (
    <div className="selector-list">
      {items.map((item) => (
        <button
          key={item.key}
          className={`selector-item ${selectedKey === item.key ? "selected" : ""}`}
          type="button"
          onClick={() => onSelect(item.key)}
        >
          <div className="selector-copy">
            <strong>{item.title}</strong>
            {item.detail ? <span>{item.detail}</span> : null}
          </div>
          <div className="selector-meta">
            {renderMeta ? renderMeta(item) : null}
            <ChevronRight size={14} />
          </div>
        </button>
      ))}
    </div>
  );
}

export function DataSparkline({ points = [], valueKey = "value", color = "var(--color-primary)" }) {
  const series = points
    .map((point) => Number(point?.[valueKey]))
    .filter((value) => Number.isFinite(value));

  if (series.length < 2) {
    return <div className="sparkline-empty">No trend</div>;
  }

  const width = 320;
  const height = 76;
  const min = Math.min(...series);
  const max = Math.max(...series);
  const range = max - min || 1;
  const step = width / Math.max(series.length - 1, 1);
  const path = series
    .map((value, index) => {
      const x = index * step;
      const y = height - ((value - min) / range) * (height - 8) - 4;
      return `${index === 0 ? "M" : "L"} ${x.toFixed(2)} ${y.toFixed(2)}`;
    })
    .join(" ");
  const area = `${path} L ${width} ${height} L 0 ${height} Z`;

  return (
    <div className="sparkline">
      <svg viewBox={`0 0 ${width} ${height}`} preserveAspectRatio="none" role="img" aria-hidden="true">
        <defs>
          <linearGradient id="spark-fill" x1="0" x2="0" y1="0" y2="1">
            <stop offset="0%" stopColor={color} stopOpacity="0.28" />
            <stop offset="100%" stopColor={color} stopOpacity="0" />
          </linearGradient>
        </defs>
        <path d={area} fill="url(#spark-fill)" />
        <path d={path} fill="none" stroke={color} strokeWidth="1.75" strokeLinecap="round" strokeLinejoin="round" />
      </svg>
    </div>
  );
}

export function BenchmarkDelta({ label, baseline, optimized, delta }) {
  const unavailable = baseline === null || baseline === undefined;
  const trend = Number(delta);
  let tone = "muted";
  if (!Number.isNaN(trend)) tone = trend > 0 ? "success" : trend < 0 ? "danger" : "muted";

  return (
    <article className={`metric-card tone-${tone}`}>
      <span className="metric-card-label">{label}</span>
      <strong className="metric-card-value mono">{unavailable ? "Unavailable" : `${baseline} -> ${optimized ?? "n/a"}`}</strong>
      <p className="metric-card-detail mono">
        {unavailable ? "No supported capture" : `Delta ${trend > 0 ? "+" : ""}${Number.isFinite(trend) ? trend : "n/a"}`}
      </p>
    </article>
  );
}

export function ToastViewport({ items, onDismiss }) {
  return (
    <div className="toast-viewport">
      {items.map((toast) => (
        <div className={`toast-card tone-${toast.tone || "muted"}`} key={toast.id}>
          <div>
            <strong>{toast.title}</strong>
            {toast.message ? <p className="text-muted">{toast.message}</p> : null}
          </div>
          <button className="icon-button subtle" type="button" onClick={() => onDismiss(toast.id)} aria-label="Dismiss">
            <X size={14} />
          </button>
        </div>
      ))}
    </div>
  );
}
