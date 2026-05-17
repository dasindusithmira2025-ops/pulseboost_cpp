import { useMemo, useState } from "react";
import {
  Gamepad2Icon,
  PlusIcon,
  UploadIcon,
} from "lucide-react";

import Card from "../components/Card";
import StatusBadge from "../components/StatusBadge";

function stabilityLabel(profile) {
  const verdict = String(profile?.history?.last_verdict || profile?.last_verdict || "").toUpperCase();
  if (verdict === "HELPED") return { label: "Excellent", color: "text-success" };
  if (verdict === "REGRESSION") return { label: "Fair", color: "text-warning" };
  return { label: "Good", color: "text-accent" };
}

export default function ProfilesPage({
  currentGameProfile,
  featureAccess,
  fileInputRef,
  gameCatalog,
  fetchGameProfile,
  handleCreateProfile,
  handleExportCurrentProfile,
  handleSaveCurrentProfile,
  refreshProfiles,
  selectedGameId,
  setCurrentGameProfile,
  setSelectedGameId,
}) {
  const [activeTab, setActiveTab] = useState("my");
  const profiles = useMemo(() => {
    return (gameCatalog || []).map((game) => {
      const stability = stabilityLabel(game);
      return {
        ...game,
        tweaks: currentGameProfile?.game_id === game.game_id
          ? currentGameProfile?.recommended_tweaks?.length || 0
          : 0,
        lastUsed: game.last_session_at ? "Observed" : "-",
        stability,
        scoreDelta: game.last_verdict === "HELPED" ? "+5.0%" : game.last_verdict === "REGRESSION" ? "-2.0%" : "0%",
        active: game.game_id === selectedGameId,
      };
    });
  }, [currentGameProfile?.game_id, currentGameProfile?.recommended_tweaks?.length, gameCatalog, selectedGameId]);

  const cloudSyncEnabled = Boolean(featureAccess?.cloud_profile_sync);
  const communityEnabled = Boolean(featureAccess?.cloud_profile_sync || featureAccess?.multi_device_license);

  return (
    <div className="space-y-5">
      <div className="flex flex-wrap items-center justify-between gap-3">
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
          Game Profiles
        </h1>
        <div className="flex items-center gap-2">
          <button
            className="flex items-center gap-1.5 rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover"
            onClick={() => fileInputRef.current?.click()}
            type="button"
          >
            <UploadIcon className="h-3.5 w-3.5" /> Import
          </button>
          <button
            className="rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover"
            onClick={() => refreshProfiles?.()}
            type="button"
          >
            Refresh
          </button>
          <button
            className="flex items-center gap-2 rounded-md bg-accent px-4 py-1.5 text-sm font-medium text-white transition-colors hover:bg-accent-hover"
            onClick={() => handleCreateProfile?.()}
            type="button"
          >
            <PlusIcon className="h-4 w-4" /> Create Profile
          </button>
        </div>
      </div>

      <div className="flex items-center gap-0 border-b border-border-subtle">
        {[
          { id: "my", label: "My Profiles" },
          { id: "recommended", label: "Recommended" },
          { id: "community", label: "Community" },
        ].map((tab) => (
          <button
            key={tab.id}
            onClick={() => setActiveTab(tab.id)}
            className={`relative px-4 py-2 text-sm transition-colors ${
              activeTab === tab.id
                ? "text-txt-primary"
                : "text-txt-tertiary hover:text-txt-secondary"
            }`}
            type="button"
          >
            {tab.label}
            {tab.id === "community" ? <span className="ml-1.5"><StatusBadge status="PREMIUM" /></span> : null}
            {activeTab === tab.id ? (
              <span className="absolute bottom-0 left-0 right-0 h-0.5 rounded-t bg-accent" />
            ) : null}
          </button>
        ))}
      </div>

      {activeTab === "my" ? (
        <div className="space-y-3">
          {profiles.length ? profiles.map((profile) => (
            <Card
              key={profile.game_id}
              className="p-5"
              statusColor={profile.active ? "#34d399" : undefined}
            >
              <div className="flex flex-col gap-4 xl:flex-row xl:items-center">
                <div className="flex h-12 w-12 shrink-0 items-center justify-center rounded-lg bg-surface-sunken text-2xl">
                  {profile.game_name?.slice(0, 1) || "G"}
                </div>
                <div className="min-w-0 flex-1">
                  <div className="mb-1 flex flex-wrap items-center gap-2">
                    <h3 className="text-sm font-semibold text-txt-primary">
                      {profile.game_name}
                    </h3>
                    {profile.active ? <StatusBadge status="ACTIVE" /> : null}
                    {cloudSyncEnabled ? <StatusBadge status="PREMIUM" /> : null}
                  </div>
                  <div className="flex flex-wrap items-center gap-4 text-xs text-txt-tertiary">
                    <span>{profile.tweaks} tweaks</span>
                    <span>Last used: {profile.lastUsed}</span>
                    <span>
                      Stability:{" "}
                      <span className={profile.stability.color}>{profile.stability.label}</span>
                    </span>
                    <span>
                      Score: <span className="numeric-tabular text-success font-medium">{profile.scoreDelta}</span>
                    </span>
                  </div>
                </div>
                <div className="flex flex-wrap items-center gap-2 xl:shrink-0">
                  <button
                    className="px-3 py-1 text-xs text-txt-tertiary transition-colors hover:text-txt-secondary"
                    onClick={async () => {
                      setSelectedGameId(profile.game_id);
                      setCurrentGameProfile(await fetchGameProfile(profile.game_id));
                    }}
                    type="button"
                  >
                    Edit
                  </button>
                  <button
                    className="px-3 py-1 text-xs text-txt-tertiary transition-colors hover:text-txt-secondary"
                    onClick={async () => {
                      setSelectedGameId(profile.game_id);
                      setCurrentGameProfile(await fetchGameProfile(profile.game_id));
                      await handleExportCurrentProfile?.(profile.game_id);
                    }}
                    type="button"
                  >
                    Export
                  </button>
                  {profile.active ? (
                    <button
                      className="rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-primary transition-colors hover:bg-surface-hover"
                      onClick={() => setSelectedGameId(null)}
                      type="button"
                    >
                      Deactivate
                    </button>
                  ) : (
                    <button
                      className="rounded-md border border-accent px-3 py-1.5 text-xs text-accent transition-colors hover:bg-accent-muted"
                      onClick={async () => {
                        setSelectedGameId(profile.game_id);
                        setCurrentGameProfile(await fetchGameProfile(profile.game_id));
                      }}
                      type="button"
                    >
                      Activate
                    </button>
                  )}
                </div>
              </div>
            </Card>
          )) : (
            <Card className="p-6 text-center text-sm text-txt-tertiary">
              No game profiles are available yet. Import one or let PulseBoost observe a session.
            </Card>
          )}

          {currentGameProfile ? (
            <Card className="p-5">
              <div className="mb-4 flex items-center justify-between">
                <h3 className="text-[15px] font-semibold text-txt-primary">
                  Selected Profile
                </h3>
                <StatusBadge status={currentGameProfile.history?.last_verdict || "MONITORED"} />
              </div>
              <div className="grid gap-4 lg:grid-cols-2">
                <label className="block">
                  <span className="mb-1.5 block text-[11px] uppercase tracking-[0.18em] text-txt-tertiary">
                    Game Name
                  </span>
                  <input
                    className="w-full rounded-md border border-border-default bg-surface-sunken px-3 py-2 text-sm text-txt-primary focus:border-accent focus:outline-none"
                    value={currentGameProfile.game_name || ""}
                    onChange={(event) =>
                      setCurrentGameProfile({ ...currentGameProfile, game_name: event.target.value })
                    }
                  />
                </label>
                <label className="block">
                  <span className="mb-1.5 block text-[11px] uppercase tracking-[0.18em] text-txt-tertiary">
                    Executable
                  </span>
                  <input
                    className="w-full rounded-md border border-border-default bg-surface-sunken px-3 py-2 text-sm text-txt-primary focus:border-accent focus:outline-none"
                    value={currentGameProfile.executable_path || ""}
                    onChange={(event) =>
                      setCurrentGameProfile({ ...currentGameProfile, executable_path: event.target.value })
                    }
                  />
                </label>
              </div>
              <label className="mt-4 block">
                <span className="mb-1.5 block text-[11px] uppercase tracking-[0.18em] text-txt-tertiary">
                  Notes
                </span>
                <textarea
                  rows="4"
                  className="w-full rounded-md border border-border-default bg-surface-sunken px-3 py-2 text-sm text-txt-primary focus:border-accent focus:outline-none"
                  value={currentGameProfile.notes || ""}
                  onChange={(event) =>
                    setCurrentGameProfile({ ...currentGameProfile, notes: event.target.value })
                  }
                />
              </label>
              <div className="mt-4 flex flex-wrap items-center gap-2">
                <button
                  className="rounded-md border border-border-default px-3 py-1.5 text-xs text-txt-secondary transition-colors hover:bg-surface-hover"
                  onClick={() => handleExportCurrentProfile?.(currentGameProfile.game_id)}
                  type="button"
                >
                  Export
                </button>
                <button
                  className="rounded-md bg-accent px-3 py-1.5 text-xs font-medium text-white transition-colors hover:bg-accent-hover"
                  onClick={() => handleSaveCurrentProfile?.()}
                  type="button"
                >
                  Save Profile
                </button>
              </div>
            </Card>
          ) : null}
        </div>
      ) : null}

      {activeTab === "recommended" ? (
        <div className="space-y-3">
          {(gameCatalog || []).slice(0, 4).map((game) => (
            <Card key={game.game_id} className="p-5">
              <div className="flex flex-col gap-4 xl:flex-row xl:items-center">
                <div className="flex h-12 w-12 items-center justify-center rounded-lg bg-surface-sunken text-2xl">
                  {game.game_name?.slice(0, 1) || "G"}
                </div>
                <div className="flex-1">
                  <div className="mb-1 flex flex-wrap items-center gap-2">
                    <h3 className="text-sm font-semibold text-txt-primary">
                      {game.game_name}
                    </h3>
                    {cloudSyncEnabled ? <StatusBadge status="PREMIUM" /> : null}
                  </div>
                  <div className="flex flex-wrap items-center gap-4 text-xs text-txt-tertiary">
                    <span>{currentGameProfile?.recommended_tweaks?.length || 0} tweaks</span>
                    <span>{game.last_verdict ? `Last verdict: ${game.last_verdict}` : "No benchmark verdict yet"}</span>
                    <span>{game.last_session_at ? "Observed locally" : "Awaiting local evidence"}</span>
                  </div>
                </div>
                <button
                  className="rounded-md border border-accent px-3 py-1.5 text-xs text-accent transition-colors hover:bg-accent-muted"
                  onClick={async () => {
                    setSelectedGameId(game.game_id);
                    setCurrentGameProfile(await fetchGameProfile(game.game_id));
                  }}
                  type="button"
                >
                  Apply
                </button>
              </div>
            </Card>
          ))}
        </div>
      ) : null}

      {activeTab === "community" ? (
        <div className="flex flex-col items-center justify-center py-16 text-center">
          <div className="mb-4 flex h-12 w-12 items-center justify-center rounded-full bg-premium-muted">
            <Gamepad2Icon className="h-6 w-6 text-premium" />
          </div>
          <h3 className="mb-2 text-[15px] font-semibold text-txt-secondary">
            Community Profiles
          </h3>
          <p className="mb-4 max-w-sm text-sm text-txt-tertiary">
            Browse and import optimization profiles shared by the PulseBoost community. Available with Pro.
          </p>
          <button
            className="rounded-md bg-premium-muted px-4 py-1.5 text-sm font-medium text-premium transition-colors hover:bg-premium/20"
            disabled={!communityEnabled}
            type="button"
          >
            {communityEnabled ? "Community sync unavailable in this desktop build" : "Upgrade to Pro"}
          </button>
        </div>
      ) : null}
    </div>
  );
}
