import { useMemo, useState } from "react";
import {
  CheckIcon,
  CrownIcon,
  KeyIcon,
  MonitorIcon,
  UserIcon,
} from "lucide-react";

import Card from "../components/Card";
import StatusBadge from "../components/StatusBadge";
import { formatDateTime } from "../utils/formatters";

const FEATURE_ROWS = [
  { key: "core_monitoring", label: "Core monitoring" },
  { key: "safe_optimizations", label: "Safe optimizations" },
  { key: "benchmark_history", label: "Benchmark history" },
  { key: "premium_benchmark_packs", label: "Premium benchmark packs" },
  { key: "advanced_gpu_controls", label: "Advanced GPU controls" },
  { key: "cloud_profile_sync", label: "Cloud profile sync" },
  { key: "multi_device_license", label: "Multi-device license" },
  { key: "enterprise_policy_templates", label: "Enterprise policy templates" },
  { key: "audit_export", label: "Audit export" },
  { key: "advanced_network_controls", label: "Advanced network controls" },
];

export default function AccountPage({
  accountLinks,
  authStatus,
  featureAccess,
  handleLocalSignIn,
  handleOpenCreateAccount,
  handleOpenManageSubscription,
  handleOpenWebsiteSignIn,
  handleRefreshEntitlements,
  handleSignOut,
  handleExportReport,
  plan,
}) {
  const [email, setEmail] = useState("");
  const [displayName, setDisplayName] = useState("");
  const [planTier, setPlanTier] = useState(plan || "free");
  const signedIn = Boolean(authStatus?.signed_in);
  const identity = authStatus?.identity;
  const currentPlan = authStatus?.plan?.plan_tier || plan || "free";
  const websiteSignInAvailable = Boolean(accountLinks?.sign_in_url);
  const createAccountAvailable = Boolean(accountLinks?.create_account_url);
  const manageSubscriptionAvailable = Boolean(accountLinks?.manage_subscription_url);
  const localDevSignInEnabled = Boolean(authStatus?.authority?.dev_mode_available);
  const featureRows = useMemo(
    () => FEATURE_ROWS.map((row) => ({
      ...row,
      included: Boolean(featureAccess?.[row.key]),
    })),
    [featureAccess],
  );

  return (
    <div className="space-y-5">
      <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
        Account
      </h1>

      <Card className="p-5">
        {signedIn ? (
          <div className="flex flex-col gap-4 md:flex-row md:items-center">
            <div className="flex h-14 w-14 shrink-0 items-center justify-center rounded-full bg-accent/20">
              <UserIcon className="h-7 w-7 text-accent" />
            </div>
            <div className="flex-1">
              <h2 className="text-lg font-semibold text-txt-primary">
                {identity?.display_name || identity?.email || "PulseBoost Operator"}
              </h2>
              <p className="text-sm text-txt-secondary">{identity?.email || "No email stored"}</p>
              <p className="mt-0.5 text-xs text-txt-tertiary">
                Member since {formatDateTime(identity?.created_at || authStatus?.session?.last_verified_at)}
              </p>
            </div>
            <div className="flex items-center gap-2">
              <button
                className="rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover"
                onClick={() => handleRefreshEntitlements?.()}
                type="button"
              >
                Refresh
              </button>
              <button
                className="px-3 py-1.5 text-xs text-txt-tertiary transition-colors hover:text-txt-secondary"
                onClick={() => handleSignOut?.()}
                type="button"
              >
                Sign Out
              </button>
            </div>
          </div>
        ) : (
          <div className="space-y-4">
            <div>
              <h2 className="text-lg font-semibold text-txt-primary">Signed out</h2>
              <p className="text-sm text-txt-secondary">
                Sign in with your website account to activate paid features and entitlement-backed controls.
              </p>
            </div>
            {localDevSignInEnabled ? (
              <div className="grid gap-4 lg:grid-cols-3">
                <label className="block">
                  <span className="mb-1.5 block text-[11px] uppercase tracking-[0.18em] text-txt-tertiary">Email</span>
                  <input
                    className="w-full rounded-md border border-border-default bg-surface-sunken px-3 py-2 text-sm text-txt-primary focus:border-accent focus:outline-none"
                    value={email}
                    onChange={(event) => setEmail(event.target.value)}
                    placeholder="operator@pulseboost.dev"
                  />
                </label>
                <label className="block">
                  <span className="mb-1.5 block text-[11px] uppercase tracking-[0.18em] text-txt-tertiary">Display Name</span>
                  <input
                    className="w-full rounded-md border border-border-default bg-surface-sunken px-3 py-2 text-sm text-txt-primary focus:border-accent focus:outline-none"
                    value={displayName}
                    onChange={(event) => setDisplayName(event.target.value)}
                    placeholder="PulseBoost Operator"
                  />
                </label>
                <label className="block">
                  <span className="mb-1.5 block text-[11px] uppercase tracking-[0.18em] text-txt-tertiary">Plan Tier</span>
                  <select
                    className="w-full rounded-md border border-border-default bg-surface-sunken px-3 py-2 text-sm text-txt-primary focus:border-accent focus:outline-none"
                    value={planTier}
                    onChange={(event) => setPlanTier(event.target.value)}
                  >
                    <option value="free">Free</option>
                    <option value="pro">Pro</option>
                    <option value="team">Team</option>
                    <option value="enterprise">Enterprise</option>
                  </select>
                </label>
              </div>
            ) : null}
            <div className="flex flex-wrap items-center gap-2">
              {localDevSignInEnabled ? (
                <button
                  className="rounded-md bg-accent px-4 py-1.5 text-sm font-medium text-white transition-colors hover:bg-accent-hover"
                  onClick={() => handleLocalSignIn?.({ email, display_name: displayName, plan_tier: planTier })}
                  type="button"
                >
                  Start Local Placeholder Session
                </button>
              ) : null}
              <button
                className="rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover disabled:cursor-not-allowed disabled:opacity-60"
                onClick={() => handleOpenWebsiteSignIn?.(accountLinks?.sign_in_url)}
                disabled={!websiteSignInAvailable}
                type="button"
              >
                Website Sign In
              </button>
              <button
                className="rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover disabled:cursor-not-allowed disabled:opacity-60"
                onClick={() => handleOpenCreateAccount?.(accountLinks?.create_account_url)}
                disabled={!createAccountAvailable}
                type="button"
              >
                Create Account
              </button>
            </div>
            {!websiteSignInAvailable && !createAccountAvailable ? (
              <p className="text-xs text-txt-tertiary">
                Website sign-in and account links are unavailable until the website authority URLs are configured.
              </p>
            ) : null}
            {localDevSignInEnabled ? (
              <p className="text-xs text-txt-tertiary">
                Local placeholder sign-in is enabled for development mode only.
              </p>
            ) : null}
          </div>
        )}
      </Card>

      <Card elevated className="p-5">
        <div className="mb-4 flex items-center justify-between">
          <h3 className="text-[15px] font-semibold text-txt-primary">Current Plan</h3>
        </div>
        <div className="flex flex-col gap-4 rounded-lg border border-accent/20 bg-accent-muted p-4 md:flex-row md:items-center">
          <div className="flex h-10 w-10 shrink-0 items-center justify-center rounded-lg bg-accent/20">
            <CrownIcon className="h-5 w-5 text-accent" />
          </div>
          <div className="flex-1">
            <div className="flex items-center gap-2">
              <h4 className="text-sm font-semibold text-txt-primary">
                PulseBoost {String(currentPlan).charAt(0).toUpperCase() + String(currentPlan).slice(1)}
              </h4>
              <StatusBadge status={signedIn ? "ACTIVE" : "SIGNED_OUT"} />
            </div>
            <div className="mt-1 flex flex-wrap items-center gap-4 text-xs text-txt-tertiary">
              <span>Last verified: {formatDateTime(authStatus?.session?.last_verified_at)}</span>
              <span>Entitlement source: {authStatus?.entitlement_snapshot?.source || "local cache"}</span>
            </div>
          </div>
          <div className="flex items-center gap-2">
            <button
              className="rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover disabled:cursor-not-allowed disabled:opacity-60"
              onClick={() => handleOpenManageSubscription?.(accountLinks?.manage_subscription_url)}
              disabled={!manageSubscriptionAvailable}
              type="button"
            >
              Manage Subscription
            </button>
          </div>
        </div>
      </Card>

      <Card className="p-5">
        <h3 className="mb-4 text-[15px] font-semibold text-txt-primary">Feature Access</h3>
        <div className="space-y-2.5">
          {featureRows.map((item) => (
            <div key={item.key} className="flex items-center justify-between py-1">
              <div className="flex items-center gap-2.5">
                {item.included ? (
                  <CheckIcon className="h-4 w-4 shrink-0 text-success" />
                ) : (
                  <span className="h-4 w-4 shrink-0 rounded-full border border-border-default" />
                )}
                <span className={`text-sm ${item.included ? "text-txt-primary" : "text-txt-tertiary"}`}>
                  {item.label}
                </span>
              </div>
              {!item.included ? <StatusBadge status="LOCKED" /> : null}
            </div>
          ))}
        </div>
      </Card>

      <Card className="p-5">
        <div className="mb-4 flex items-center justify-between">
          <h3 className="text-[15px] font-semibold text-txt-primary">Devices</h3>
          <span className="text-xs text-txt-tertiary">
            {authStatus?.activation?.activation_id ? "1 active device" : "No active device"}
          </span>
        </div>
        <div className="space-y-3">
          <div className="flex items-center gap-3 rounded-md bg-surface-sunken p-3">
            <MonitorIcon className="h-5 w-5 shrink-0 text-accent" />
            <div className="flex-1">
              <div className="flex items-center gap-2">
                <span className="text-sm font-medium text-txt-primary">
                  {authStatus?.activation?.device_name || "This PC"}
                </span>
                <span className="numeric-tabular text-[11px] text-txt-tertiary">
                  {authStatus?.activation?.activation_id || "Pending activation"}
                </span>
              </div>
            </div>
            <span className={`text-xs ${authStatus?.activation?.revoked ? "text-error" : "text-success"}`}>
              {authStatus?.activation?.revoked ? "Revoked" : "Active"}
            </span>
          </div>
        </div>
      </Card>

      <Card className="p-5">
        <h3 className="mb-4 text-[15px] font-semibold text-txt-primary">License</h3>
        <div className="flex items-center gap-3 rounded-md bg-surface-sunken p-3">
          <KeyIcon className="h-5 w-5 shrink-0 text-txt-tertiary" />
          <div className="flex-1">
            <div className="flex items-center gap-2">
              <span className="numeric-tabular text-sm text-txt-primary">
                {authStatus?.activation?.activation_id || "Website-managed license; no local key is stored"}
              </span>
            </div>
            <div className="mt-1 flex flex-wrap items-center gap-3 text-xs text-txt-tertiary">
              <span>Activated: {formatDateTime(authStatus?.activation?.activated_at)}</span>
              <span>Type: {signedIn ? authStatus?.session?.source || "local" : "signed out"}</span>
            </div>
          </div>
        </div>
        <div className="mt-4 border-t border-border-subtle pt-3">
          <button
            type="button"
            onClick={() => handleExportReport?.()}
            className="rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover"
          >
            Export Report
          </button>
        </div>
      </Card>
    </div>
  );
}
