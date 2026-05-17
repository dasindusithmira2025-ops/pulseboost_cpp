```App.tsx
import React, { useState } from 'react'
import { Sidebar } from './components/Sidebar'
import { Dashboard } from './pages/Dashboard'
import { Optimizations } from './pages/Optimizations'
import { Network } from './pages/Network'
import { GPU } from './pages/GPU'
import { AuditLog } from './pages/AuditLog'
import { Benchmark } from './pages/Benchmark'
import { GameProfiles } from './pages/GameProfiles'
import { TrustCenter } from './pages/TrustCenter'
import { Settings } from './pages/Settings'
import { Account } from './pages/Account'
import { MinusIcon, SquareIcon, XIcon, ZapIcon } from 'lucide-react'
type Page =
  | 'dashboard'
  | 'optimizations'
  | 'network'
  | 'gpu'
  | 'audit'
  | 'benchmark'
  | 'profiles'
  | 'trust'
  | 'settings'
  | 'account'
export function App() {
  const [activePage, setActivePage] = useState<Page>('dashboard')
  const [sidebarCollapsed, setSidebarCollapsed] = useState(false)
  const renderPage = () => {
    switch (activePage) {
      case 'dashboard':
        return <Dashboard onNavigate={(p) => setActivePage(p as Page)} />
      case 'optimizations':
        return <Optimizations />
      case 'network':
        return <Network />
      case 'gpu':
        return <GPU />
      case 'audit':
        return <AuditLog />
      case 'benchmark':
        return <Benchmark />
      case 'profiles':
        return <GameProfiles />
      case 'trust':
        return <TrustCenter />
      case 'settings':
        return <Settings />
      case 'account':
        return <Account />
      default:
        return <Dashboard onNavigate={(p) => setActivePage(p as Page)} />
    }
  }
  return (
    <div
      className="flex flex-col w-full h-screen bg-app text-txt-primary overflow-hidden"
      style={{
        fontFamily: "'Inter', sans-serif",
      }}
    >
      {/* Custom Title Bar */}
      <header className="flex items-center justify-between h-8 px-3 bg-app border-b border-border-subtle flex-shrink-0 select-none">
        <div className="flex items-center gap-2">
          <div className="w-4 h-4 rounded bg-accent/20 flex items-center justify-center">
            <ZapIcon className="w-2.5 h-2.5 text-accent" />
          </div>
          <span className="text-xs font-semibold text-txt-tertiary tracking-wide">
            PulseBoost
          </span>
        </div>
        <div className="flex items-center">
          <button className="w-8 h-8 flex items-center justify-center text-txt-tertiary hover:text-txt-secondary hover:bg-surface-hover transition-colors">
            <MinusIcon className="w-3.5 h-3.5" />
          </button>
          <button className="w-8 h-8 flex items-center justify-center text-txt-tertiary hover:text-txt-secondary hover:bg-surface-hover transition-colors">
            <SquareIcon className="w-3 h-3" />
          </button>
          <button className="w-8 h-8 flex items-center justify-center text-txt-tertiary hover:text-error hover:bg-error-muted transition-colors">
            <XIcon className="w-3.5 h-3.5" />
          </button>
        </div>
      </header>

      {/* Main Layout */}
      <div className="flex flex-1 overflow-hidden">
        <Sidebar
          activePage={activePage}
          onNavigate={setActivePage}
          collapsed={sidebarCollapsed}
          onToggleCollapse={() => setSidebarCollapsed(!sidebarCollapsed)}
        />
        <main className="flex-1 overflow-y-auto p-6">
          <div className="max-w-[1200px] mx-auto">{renderPage()}</div>
        </main>
      </div>
    </div>
  )
}

```
```components/Card.tsx
import React from 'react'
interface CardProps {
  children: React.ReactNode
  className?: string
  elevated?: boolean
  statusColor?: string
  onClick?: () => void
}
export function Card({
  children,
  className = '',
  elevated = false,
  statusColor,
  onClick,
}: CardProps) {
  return (
    <div
      onClick={onClick}
      className={`
        rounded-lg border border-border-default
        ${elevated ? 'bg-surface-elevated shadow-lg shadow-black/20' : 'bg-surface'}
        ${statusColor ? 'border-l-[3px]' : ''}
        ${onClick ? 'cursor-pointer hover:bg-surface-hover transition-colors' : ''}
        ${className}
      `}
      style={
        statusColor
          ? {
              borderLeftColor: statusColor,
            }
          : undefined
      }
    >
      {children}
    </div>
  )
}

```
```components/HealthRing.tsx
import React from 'react'
interface HealthRingProps {
  score: number
  size?: number
  strokeWidth?: number
}
export function HealthRing({
  score,
  size = 64,
  strokeWidth = 5,
}: HealthRingProps) {
  const radius = (size - strokeWidth) / 2
  const circumference = 2 * Math.PI * radius
  const progress = (score / 100) * circumference
  const color = score >= 80 ? '#34d399' : score >= 60 ? '#fbbf24' : '#f87171'
  return (
    <div
      className="relative"
      style={{
        width: size,
        height: size,
      }}
    >
      <svg width={size} height={size} className="-rotate-90">
        <circle
          cx={size / 2}
          cy={size / 2}
          r={radius}
          fill="none"
          stroke="#1f2231"
          strokeWidth={strokeWidth}
        />
        <circle
          cx={size / 2}
          cy={size / 2}
          r={radius}
          fill="none"
          stroke={color}
          strokeWidth={strokeWidth}
          strokeLinecap="round"
          strokeDasharray={circumference}
          strokeDashoffset={circumference - progress}
          className="transition-all duration-700 ease-out"
        />
      </svg>
      <div className="absolute inset-0 flex items-center justify-center">
        <span className="text-lg font-semibold text-txt-primary">{score}</span>
      </div>
    </div>
  )
}

```
```components/MiniChart.tsx
import React from 'react'
interface MiniChartProps {
  data: number[]
  color?: string
  width?: number
  height?: number
}
export function MiniChart({
  data,
  color = '#6366f1',
  width = 80,
  height = 32,
}: MiniChartProps) {
  if (data.length < 2) return null
  const min = Math.min(...data)
  const max = Math.max(...data)
  const range = max - min || 1
  const points = data
    .map((v, i) => {
      const x = (i / (data.length - 1)) * width
      const y = height - ((v - min) / range) * (height - 4) - 2
      return `${x},${y}`
    })
    .join(' ')
  const areaPoints = `0,${height} ${points} ${width},${height}`
  return (
    <svg width={width} height={height} className="flex-shrink-0">
      <defs>
        <linearGradient
          id={`grad-${color.replace('#', '')}`}
          x1="0"
          y1="0"
          x2="0"
          y2="1"
        >
          <stop offset="0%" stopColor={color} stopOpacity="0.2" />
          <stop offset="100%" stopColor={color} stopOpacity="0" />
        </linearGradient>
      </defs>
      <polygon
        points={areaPoints}
        fill={`url(#grad-${color.replace('#', '')})`}
      />
      <polyline
        points={points}
        fill="none"
        stroke={color}
        strokeWidth="1.5"
        strokeLinecap="round"
        strokeLinejoin="round"
      />
    </svg>
  )
}

```
```components/Sidebar.tsx
import React from 'react'
import {
  LayoutDashboardIcon,
  SlidersHorizontalIcon,
  GlobeIcon,
  CpuIcon,
  FileTextIcon,
  BarChart3Icon,
  Gamepad2Icon,
  ShieldIcon,
  SettingsIcon,
  UserIcon,
  ChevronLeftIcon,
  ChevronRightIcon,
  SearchIcon,
} from 'lucide-react'
type Page =
  | 'dashboard'
  | 'optimizations'
  | 'network'
  | 'gpu'
  | 'audit'
  | 'benchmark'
  | 'profiles'
  | 'trust'
  | 'settings'
  | 'account'
const navItems: {
  id: Page
  label: string
  icon: React.ElementType
}[] = [
  {
    id: 'dashboard',
    label: 'Dashboard',
    icon: LayoutDashboardIcon,
  },
  {
    id: 'optimizations',
    label: 'Optimizations',
    icon: SlidersHorizontalIcon,
  },
  {
    id: 'network',
    label: 'Network',
    icon: GlobeIcon,
  },
  {
    id: 'gpu',
    label: 'GPU',
    icon: CpuIcon,
  },
  {
    id: 'audit',
    label: 'Audit Log',
    icon: FileTextIcon,
  },
  {
    id: 'benchmark',
    label: 'Benchmark',
    icon: BarChart3Icon,
  },
  {
    id: 'profiles',
    label: 'Game Profiles',
    icon: Gamepad2Icon,
  },
  {
    id: 'trust',
    label: 'Trust Center',
    icon: ShieldIcon,
  },
]
interface SidebarProps {
  activePage: Page
  onNavigate: (page: Page) => void
  collapsed: boolean
  onToggleCollapse: () => void
}
export function Sidebar({
  activePage,
  onNavigate,
  collapsed,
  onToggleCollapse,
}: SidebarProps) {
  return (
    <aside
      className={`flex flex-col h-full bg-nav border-r border-border-subtle transition-all duration-200 ${collapsed ? 'w-14' : 'w-[220px]'}`}
    >
      {/* Search */}
      <div className="px-2 pt-3 pb-1">
        <button
          onClick={() => {}}
          className={`flex items-center gap-2 w-full rounded-md px-2.5 py-1.5 text-txt-tertiary hover:text-txt-secondary hover:bg-surface-hover transition-colors ${collapsed ? 'justify-center' : ''}`}
        >
          <SearchIcon className="w-4 h-4 flex-shrink-0" />
          {!collapsed && <span className="text-xs">Search...</span>}
          {!collapsed && (
            <span className="ml-auto text-[10px] text-txt-disabled border border-border-default rounded px-1">
              ⌘K
            </span>
          )}
        </button>
      </div>

      {/* Primary Nav */}
      <nav className="flex-1 px-2 py-2 space-y-0.5 overflow-y-auto">
        {navItems.map((item) => {
          const Icon = item.icon
          const isActive = activePage === item.id
          return (
            <button
              key={item.id}
              onClick={() => onNavigate(item.id)}
              className={`flex items-center gap-2.5 w-full rounded-md px-2.5 py-2 text-[13px] transition-colors relative group
                ${isActive ? 'bg-nav-active text-txt-primary' : 'text-txt-secondary hover:text-txt-primary hover:bg-surface-hover'}
                ${collapsed ? 'justify-center' : ''}
              `}
              title={collapsed ? item.label : undefined}
            >
              {isActive && (
                <span className="absolute left-0 top-1/2 -translate-y-1/2 w-0.5 h-4 bg-accent rounded-r" />
              )}
              <Icon
                className={`w-4 h-4 flex-shrink-0 ${isActive ? 'text-accent' : ''}`}
              />
              {!collapsed && <span>{item.label}</span>}
            </button>
          )
        })}
      </nav>

      {/* Bottom Section */}
      <div className="px-2 pb-3 space-y-0.5 border-t border-border-subtle pt-2">
        <button
          onClick={() => onNavigate('settings')}
          className={`flex items-center gap-2.5 w-full rounded-md px-2.5 py-2 text-[13px] transition-colors
            ${activePage === 'settings' ? 'bg-nav-active text-txt-primary' : 'text-txt-secondary hover:text-txt-primary hover:bg-surface-hover'}
            ${collapsed ? 'justify-center' : ''}
          `}
          title={collapsed ? 'Settings' : undefined}
        >
          <SettingsIcon
            className={`w-4 h-4 flex-shrink-0 ${activePage === 'settings' ? 'text-accent' : ''}`}
          />
          {!collapsed && <span>Settings</span>}
        </button>

        {/* Account Pill */}
        <button
          onClick={() => onNavigate('account')}
          className={`flex items-center gap-2.5 w-full rounded-md px-2.5 py-2 transition-colors hover:bg-surface-hover
            ${collapsed ? 'justify-center' : ''}
          `}
        >
          <div className="w-7 h-7 rounded-full bg-accent/20 flex items-center justify-center flex-shrink-0">
            <UserIcon className="w-3.5 h-3.5 text-accent" />
          </div>
          {!collapsed && (
            <div className="flex-1 min-w-0 text-left">
              <div className="text-xs text-txt-primary truncate">John Doe</div>
              <div className="text-[10px] text-accent font-semibold uppercase tracking-wider">
                PRO
              </div>
            </div>
          )}
        </button>

        {/* Collapse Toggle */}
        <button
          onClick={onToggleCollapse}
          className="flex items-center justify-center w-full rounded-md py-1.5 text-txt-tertiary hover:text-txt-secondary hover:bg-surface-hover transition-colors"
        >
          {collapsed ? (
            <ChevronRightIcon className="w-4 h-4" />
          ) : (
            <ChevronLeftIcon className="w-4 h-4" />
          )}
        </button>
      </div>
    </aside>
  )
}

```
```components/StatusBadge.tsx
import React from 'react'
import { LockIcon, SparklesIcon } from 'lucide-react'
export type BadgeStatus =
  | 'VALIDATED'
  | 'LEGACY'
  | 'HARDWARE_SPECIFIC'
  | 'PLACEBO_RISK'
  | 'ACTIVE'
  | 'REVERTED'
  | 'FAILED'
  | 'AI_APPLIED'
  | 'TEMPORARY'
  | 'REQUIRES_ADMIN'
  | 'UNSUPPORTED'
  | 'EXPERT_ONLY'
  | 'DRY_RUN'
  | 'LIVE'
  | 'SAFE_DEFAULT'
  | 'RECOVERY_MODE'
  | 'LOCKED'
  | 'PREMIUM'
const badgeConfig: Record<
  BadgeStatus,
  {
    bg: string
    text: string
    dot?: string
    icon?: 'lock' | 'sparkle'
    pulse?: boolean
  }
> = {
  VALIDATED: {
    bg: 'bg-success-muted',
    text: 'text-success',
    dot: 'bg-success',
  },
  LEGACY: {
    bg: 'bg-surface-hover',
    text: 'text-txt-tertiary',
  },
  HARDWARE_SPECIFIC: {
    bg: 'bg-info-muted',
    text: 'text-info',
  },
  PLACEBO_RISK: {
    bg: 'bg-warning-muted',
    text: 'text-warning',
  },
  ACTIVE: {
    bg: 'bg-success-muted',
    text: 'text-success',
    dot: 'bg-success',
  },
  REVERTED: {
    bg: 'bg-surface-hover',
    text: 'text-txt-tertiary',
  },
  FAILED: {
    bg: 'bg-error-muted',
    text: 'text-error',
    dot: 'bg-error',
  },
  AI_APPLIED: {
    bg: 'bg-accent-muted',
    text: 'text-accent',
  },
  TEMPORARY: {
    bg: 'bg-info-muted',
    text: 'text-info',
  },
  REQUIRES_ADMIN: {
    bg: 'bg-warning-muted',
    text: 'text-warning',
  },
  UNSUPPORTED: {
    bg: 'bg-surface-hover',
    text: 'text-txt-tertiary',
  },
  EXPERT_ONLY: {
    bg: 'bg-warning-muted',
    text: 'text-warning',
  },
  DRY_RUN: {
    bg: 'bg-info-muted',
    text: 'text-info',
  },
  LIVE: {
    bg: 'bg-success-muted',
    text: 'text-success',
    dot: 'bg-success',
    pulse: true,
  },
  SAFE_DEFAULT: {
    bg: 'bg-success-muted',
    text: 'text-success',
  },
  RECOVERY_MODE: {
    bg: 'bg-error-muted',
    text: 'text-error',
    dot: 'bg-error',
    pulse: true,
  },
  LOCKED: {
    bg: 'bg-surface-hover',
    text: 'text-txt-tertiary',
    icon: 'lock',
  },
  PREMIUM: {
    bg: 'bg-premium-muted',
    text: 'text-premium',
    icon: 'sparkle',
  },
}
export function StatusBadge({ status }: { status: BadgeStatus }) {
  const config = badgeConfig[status]
  const label = status.replace(/_/g, ' ')
  return (
    <span
      className={`inline-flex items-center gap-1 px-2 py-0.5 rounded ${config.bg} ${config.text} text-[10px] font-semibold uppercase tracking-wider`}
    >
      {config.dot && (
        <span
          className={`w-1.5 h-1.5 rounded-full ${config.dot} ${config.pulse ? 'animate-pulse-dot' : ''}`}
        />
      )}
      {config.icon === 'lock' && <LockIcon className="w-2.5 h-2.5" />}
      {config.icon === 'sparkle' && <SparklesIcon className="w-2.5 h-2.5" />}
      {label}
    </span>
  )
}

```
```index.css
/* @import url() FONT IMPORTS MUST ALWAYS BE AT THE VERY TOP OF THIS FILE, ABOVE THE TAILWIND IMPORTS — DO NOT DELETE THIS COMMENT */
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap');

/* CRITICAL: THE FOLLOWING TAILWIND IMPORTS MUST NEVER BE DELETED OR REORDERED — DO NOT DELETE THIS COMMENT */
@import 'tailwindcss/base';
@import 'tailwindcss/components';
@import 'tailwindcss/utilities';

/* END TAILWIND IMPORTS — ALL OTHER CSS MUST GO BELOW THIS LINE */

:root {
  --bg-app: #0f1117;
  --bg-surface: #1a1d27;
  --bg-surface-elevated: #1f2231;
  --bg-surface-hover: #252838;
  --bg-surface-active: #2a2e40;
  --bg-surface-sunken: #13151c;
  --bg-nav: #14161f;
  --bg-nav-active: #1c1f2d;
  --border-default: #2a2d3a;
  --border-subtle: #1f2231;
  --border-strong: #3a3d4a;
  --text-primary: #e8eaf0;
  --text-secondary: #9ca0b0;
  --text-tertiary: #6b7084;
  --text-disabled: #4a4e60;
  --accent-primary: #6366f1;
  --accent-primary-hover: #7577f5;
  --status-success: #34d399;
  --status-warning: #fbbf24;
  --status-error: #f87171;
  --status-info: #60a5fa;
  --status-premium: #a78bfa;
}
* {
  scrollbar-width: thin;
  scrollbar-color: var(--border-default) transparent;
}
*::-webkit-scrollbar {
  width: 6px;
}
*::-webkit-scrollbar-track {
  background: transparent;
}
*::-webkit-scrollbar-thumb {
  background-color: var(--border-default);
  border-radius: 3px;
}
body {
  font-family: 'Inter', -apple-system, BlinkMacSystemFont, sans-serif;
  background: var(--bg-app);
  color: var(--text-primary);
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
}
@keyframes pulse-dot {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.4; }
}
@keyframes skeleton-pulse {
  0%, 100% { opacity: 0.06; }
  50% { opacity: 0.12; }
}
.animate-pulse-dot {
  animation: pulse-dot 2s ease-in-out infinite;
}
.animate-skeleton {
  animation: skeleton-pulse 0.8s ease-in-out infinite;
}

```
```index.tsx
import "./index.css";
import React from "react";
import { render } from "react-dom";
import { App } from "./App";

render(<App />, document.getElementById("root"));

```
```pages/Account.tsx
import React from 'react'
import {
  UserIcon,
  CheckIcon,
  CrownIcon,
  MonitorIcon,
  KeyIcon,
} from 'lucide-react'
import { Card } from '../components/Card'
import { StatusBadge } from '../components/StatusBadge'
export function Account() {
  return (
    <div className="space-y-5">
      <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
        Account
      </h1>

      {/* Identity */}
      <Card className="p-5">
        <div className="flex items-center gap-4">
          <div className="w-14 h-14 rounded-full bg-accent/20 flex items-center justify-center flex-shrink-0">
            <UserIcon className="w-7 h-7 text-accent" />
          </div>
          <div className="flex-1">
            <h2 className="text-lg font-semibold text-txt-primary">John Doe</h2>
            <p className="text-sm text-txt-secondary">john@example.com</p>
            <p className="text-xs text-txt-tertiary mt-0.5">
              Member since January 2024
            </p>
          </div>
          <div className="flex items-center gap-2">
            <button className="px-3 py-1.5 text-xs border border-border-default text-txt-secondary rounded-md hover:bg-surface-hover transition-colors">
              Edit Profile
            </button>
            <button className="px-3 py-1.5 text-xs text-txt-tertiary hover:text-txt-secondary transition-colors">
              Sign Out
            </button>
          </div>
        </div>
      </Card>

      {/* Current Plan */}
      <Card elevated className="p-5">
        <div className="flex items-center justify-between mb-4">
          <h3 className="text-[15px] font-semibold text-txt-primary">
            Current Plan
          </h3>
        </div>
        <div className="flex items-center gap-4 p-4 rounded-lg bg-accent-muted border border-accent/20">
          <div className="w-10 h-10 rounded-lg bg-accent/20 flex items-center justify-center flex-shrink-0">
            <CrownIcon className="w-5 h-5 text-accent" />
          </div>
          <div className="flex-1">
            <div className="flex items-center gap-2">
              <h4 className="text-sm font-semibold text-txt-primary">
                PulseBoost Pro
              </h4>
              <StatusBadge status="ACTIVE" />
            </div>
            <div className="flex items-center gap-4 text-xs text-txt-tertiary mt-1">
              <span>Renews: Feb 15, 2025</span>
              <span>Monthly · $9.99/mo</span>
            </div>
          </div>
          <div className="flex items-center gap-2">
            <button className="px-3 py-1.5 text-xs border border-border-default text-txt-secondary rounded-md hover:bg-surface-hover transition-colors">
              Manage Subscription
            </button>
            <button className="px-3 py-1.5 text-xs text-txt-tertiary hover:text-txt-secondary transition-colors">
              Invoice History
            </button>
          </div>
        </div>
      </Card>

      {/* Feature Access */}
      <Card className="p-5">
        <h3 className="text-[15px] font-semibold text-txt-primary mb-4">
          Feature Access
        </h3>
        <div className="space-y-2.5">
          {[
            {
              feature: 'Core optimizations (unlimited)',
              included: true,
            },
            {
              feature: 'Advanced optimizations',
              included: true,
            },
            {
              feature: 'AI adaptive engine',
              included: true,
            },
            {
              feature: 'Game profiles (unlimited)',
              included: true,
            },
            {
              feature: 'Community profiles',
              included: true,
            },
            {
              feature: 'Full benchmark & comparison',
              included: true,
            },
            {
              feature: 'Unlimited audit log',
              included: true,
            },
            {
              feature: 'Priority support',
              included: true,
            },
            {
              feature: 'Enterprise API access',
              included: false,
              tier: 'Enterprise',
            },
            {
              feature: 'Multi-device management',
              included: false,
              tier: 'Enterprise',
            },
          ].map((item) => (
            <div
              key={item.feature}
              className="flex items-center justify-between py-1"
            >
              <div className="flex items-center gap-2.5">
                {item.included ? (
                  <CheckIcon className="w-4 h-4 text-success flex-shrink-0" />
                ) : (
                  <span className="w-4 h-4 rounded-full border border-border-default flex-shrink-0" />
                )}
                <span
                  className={`text-sm ${item.included ? 'text-txt-primary' : 'text-txt-tertiary'}`}
                >
                  {item.feature}
                </span>
              </div>
              {item.tier && (
                <button className="text-[10px] font-semibold uppercase tracking-wider text-premium bg-premium-muted px-2 py-0.5 rounded hover:bg-premium/20 transition-colors">
                  {item.tier} →
                </button>
              )}
            </div>
          ))}
        </div>
      </Card>

      {/* Devices */}
      <Card className="p-5">
        <div className="flex items-center justify-between mb-4">
          <h3 className="text-[15px] font-semibold text-txt-primary">
            Devices
          </h3>
          <span className="text-xs text-txt-tertiary">2 of 3 devices used</span>
        </div>
        <div className="space-y-3">
          {[
            {
              name: 'This PC',
              id: 'DESKTOP-ABC123',
              active: true,
              lastSeen: 'Now',
            },
            {
              name: 'Gaming Laptop',
              id: 'LAPTOP-XYZ789',
              active: false,
              lastSeen: '3d ago',
            },
          ].map((device) => (
            <div
              key={device.id}
              className="flex items-center gap-3 p-3 rounded-md bg-surface-sunken"
            >
              <MonitorIcon
                className={`w-5 h-5 ${device.active ? 'text-accent' : 'text-txt-tertiary'} flex-shrink-0`}
              />
              <div className="flex-1">
                <div className="flex items-center gap-2">
                  <span className="text-sm font-medium text-txt-primary">
                    {device.name}
                  </span>
                  <span className="text-[11px] font-mono text-txt-tertiary">
                    {device.id}
                  </span>
                </div>
              </div>
              <span
                className={`text-xs ${device.active ? 'text-success' : 'text-txt-tertiary'}`}
              >
                {device.active ? 'Active' : `Last seen: ${device.lastSeen}`}
              </span>
            </div>
          ))}
        </div>
        <button className="mt-3 text-xs text-accent hover:text-accent-hover transition-colors">
          Manage Devices
        </button>
      </Card>

      {/* License */}
      <Card className="p-5">
        <h3 className="text-[15px] font-semibold text-txt-primary mb-4">
          License
        </h3>
        <div className="flex items-center gap-3 p-3 rounded-md bg-surface-sunken">
          <KeyIcon className="w-5 h-5 text-txt-tertiary flex-shrink-0" />
          <div className="flex-1">
            <div className="flex items-center gap-2">
              <span className="text-sm font-mono text-txt-primary">
                XXXX-XXXX-XXXX-XXXX
              </span>
              <button className="text-[10px] text-accent hover:text-accent-hover">
                Copy
              </button>
            </div>
            <div className="flex items-center gap-3 text-xs text-txt-tertiary mt-1">
              <span>Activated: Jan 15, 2024</span>
              <span>Type: Personal</span>
            </div>
          </div>
          <button className="px-3 py-1.5 text-xs border border-error/30 text-error rounded-md hover:bg-error-muted transition-colors">
            Deactivate
          </button>
        </div>
      </Card>
    </div>
  )
}

```
```pages/AuditLog.tsx
import React, { useState } from 'react'
import {
  SearchIcon,
  DownloadIcon,
  Trash2Icon,
  CpuIcon,
  ZapIcon,
  BarChart3Icon,
  RotateCcwIcon,
} from 'lucide-react'
import { Card } from '../components/Card'
import { StatusBadge, BadgeStatus } from '../components/StatusBadge'
interface AuditEvent {
  id: string
  time: string
  title: string
  badges: BadgeStatus[]
  source: string
  beforeAfter?: string
  type: 'applied' | 'reverted' | 'failed' | 'benchmark' | 'ai'
  group?: string
}
const events: AuditEvent[] = [
  {
    id: '1',
    time: '14:32',
    title: 'Applied "High Performance Power Plan"',
    badges: ['VALIDATED', 'ACTIVE'],
    source: 'Manual',
    beforeAfter: 'Balanced → High Performance',
    type: 'applied',
    group: 'Today',
  },
  {
    id: '2',
    time: '14:28',
    title: 'AI applied 3 network optimizations',
    badges: ['AI_APPLIED', 'ACTIVE'],
    source: 'Adaptive Engine',
    beforeAfter: "Nagle's Algorithm, TCP Window, RSS",
    type: 'ai',
    group: 'Today',
  },
  {
    id: '3',
    time: '12:15',
    title: 'Reverted "Disable Prefetch"',
    badges: ['REVERTED'],
    source: 'Manual',
    type: 'reverted',
    group: 'Today',
  },
  {
    id: '4',
    time: '10:00',
    title: 'Applied "GPU Shader Cache Optimization"',
    badges: ['VALIDATED', 'AI_APPLIED', 'ACTIVE'],
    source: 'Adaptive Engine',
    type: 'ai',
    group: 'Today',
  },
  {
    id: '5',
    time: '22:00',
    title: 'Benchmark completed',
    badges: [],
    source: 'Manual',
    beforeAfter: 'Score: 8,420 → 8,890 (+5.6%)',
    type: 'benchmark',
    group: 'Yesterday',
  },
  {
    id: '6',
    time: '18:30',
    title: 'Applied "Disable Windows Search Indexing"',
    badges: ['VALIDATED', 'ACTIVE'],
    source: 'Manual',
    type: 'applied',
    group: 'Yesterday',
  },
  {
    id: '7',
    time: '15:00',
    title: 'Failed to apply "Game Mode Enhancement"',
    badges: ['FAILED'],
    source: 'Manual',
    type: 'failed',
    group: 'Yesterday',
  },
  {
    id: '8',
    time: '12:00',
    title: 'Applied "SSD TRIM Optimization"',
    badges: ['SAFE_DEFAULT', 'ACTIVE'],
    source: 'Manual',
    type: 'applied',
    group: 'Jan 13',
  },
]
type ViewMode = 'timeline' | 'list'
export function AuditLog() {
  const [viewMode, setViewMode] = useState<ViewMode>('timeline')
  const [search, setSearch] = useState('')
  const dotColor = (type: AuditEvent['type']) => {
    switch (type) {
      case 'applied':
        return 'bg-accent'
      case 'reverted':
        return 'bg-surface-active'
      case 'failed':
        return 'bg-error'
      case 'benchmark':
        return 'border-2 border-info bg-transparent'
      case 'ai':
        return 'bg-accent border border-accent/50'
    }
  }
  const filtered = events.filter(
    (e) => !search || e.title.toLowerCase().includes(search.toLowerCase()),
  )
  const groups = [...new Set(filtered.map((e) => e.group))]
  return (
    <div className="space-y-5">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
          Audit Log
        </h1>
        <div className="flex items-center gap-2">
          <button className="px-3 py-1.5 text-xs border border-border-default text-txt-secondary rounded-md hover:bg-surface-hover transition-colors flex items-center gap-1.5">
            <DownloadIcon className="w-3.5 h-3.5" /> Export
          </button>
        </div>
      </div>

      {/* Controls */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-0 border-b border-border-subtle">
          {(['timeline', 'list'] as ViewMode[]).map((mode) => (
            <button
              key={mode}
              onClick={() => setViewMode(mode)}
              className={`px-4 py-2 text-sm capitalize transition-colors relative ${viewMode === mode ? 'text-txt-primary' : 'text-txt-tertiary hover:text-txt-secondary'}`}
            >
              {mode}
              {viewMode === mode && (
                <span className="absolute bottom-0 left-0 right-0 h-0.5 bg-accent rounded-t" />
              )}
            </button>
          ))}
        </div>
        <div className="relative">
          <SearchIcon className="absolute left-2.5 top-1/2 -translate-y-1/2 w-3.5 h-3.5 text-txt-tertiary" />
          <input
            type="text"
            value={search}
            onChange={(e) => setSearch(e.target.value)}
            placeholder="Search events..."
            className="pl-8 pr-3 py-1.5 text-sm bg-surface-sunken border border-border-default rounded-md text-txt-primary placeholder:text-txt-tertiary focus:outline-none focus:border-accent w-56"
          />
        </div>
      </div>

      {/* Timeline View */}
      {viewMode === 'timeline' && (
        <div className="space-y-6">
          {groups.map((group) => (
            <div key={group}>
              <div className="text-xs font-semibold text-txt-tertiary uppercase tracking-wider mb-3">
                {group}
              </div>
              <div className="relative pl-6">
                {/* Timeline line */}
                <div className="absolute left-[7px] top-2 bottom-2 w-px bg-border-default" />

                <div className="space-y-4">
                  {filtered
                    .filter((e) => e.group === group)
                    .map((event) => (
                      <div key={event.id} className="relative">
                        {/* Dot */}
                        <div
                          className={`absolute -left-6 top-1.5 w-3.5 h-3.5 rounded-full ${dotColor(event.type)}`}
                        />

                        <Card className="p-4 ml-2">
                          <div className="flex items-start justify-between gap-3">
                            <div className="flex-1 min-w-0">
                              <div className="flex items-center gap-2 mb-1">
                                <span className="text-[11px] font-mono text-txt-tertiary">
                                  {event.time}
                                </span>
                                <h4 className="text-sm font-medium text-txt-primary truncate">
                                  {event.title}
                                </h4>
                              </div>
                              <div className="flex items-center gap-2 mb-1">
                                {event.badges.map((b) => (
                                  <StatusBadge key={b} status={b} />
                                ))}
                                <span className="text-[11px] text-txt-tertiary">
                                  · {event.source}
                                </span>
                              </div>
                              {event.beforeAfter && (
                                <div className="text-xs font-mono text-txt-tertiary mt-1">
                                  {event.beforeAfter}
                                </div>
                              )}
                            </div>
                            <div className="flex items-center gap-2 flex-shrink-0">
                              {event.type === 'applied' && (
                                <button className="px-2 py-1 text-[11px] text-txt-tertiary hover:text-txt-secondary transition-colors flex items-center gap-1">
                                  <RotateCcwIcon className="w-3 h-3" /> Revert
                                </button>
                              )}
                              <button className="text-[11px] text-txt-tertiary hover:text-txt-secondary transition-colors">
                                Details →
                              </button>
                            </div>
                          </div>
                        </Card>
                      </div>
                    ))}
                </div>
              </div>
            </div>
          ))}
        </div>
      )}

      {/* List View */}
      {viewMode === 'list' && (
        <Card className="overflow-hidden">
          <table className="w-full">
            <thead>
              <tr className="bg-surface-sunken">
                <th className="text-left text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                  Time
                </th>
                <th className="text-left text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                  Action
                </th>
                <th className="text-left text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                  Status
                </th>
                <th className="text-left text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                  Source
                </th>
                <th className="text-right text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                  Actions
                </th>
              </tr>
            </thead>
            <tbody>
              {filtered.map((event) => (
                <tr
                  key={event.id}
                  className="border-t border-border-subtle hover:bg-surface-hover transition-colors"
                >
                  <td className="px-4 py-3 text-xs font-mono text-txt-tertiary whitespace-nowrap">
                    {event.group} {event.time}
                  </td>
                  <td className="px-4 py-3 text-sm text-txt-primary">
                    {event.title}
                  </td>
                  <td className="px-4 py-3">
                    <div className="flex gap-1">
                      {event.badges.map((b) => (
                        <StatusBadge key={b} status={b} />
                      ))}
                    </div>
                  </td>
                  <td className="px-4 py-3 text-xs text-txt-tertiary">
                    {event.source}
                  </td>
                  <td className="px-4 py-3 text-right">
                    <button className="text-[11px] text-txt-tertiary hover:text-txt-secondary">
                      Details →
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </Card>
      )}
    </div>
  )
}

```
```pages/Benchmark.tsx
import React, { useState } from 'react'
import {
  PlayIcon,
  TrendingUpIcon,
  TrendingDownIcon,
  MinusIcon,
} from 'lucide-react'
import { Card } from '../components/Card'
import { MiniChart } from '../components/MiniChart'
type Tab = 'latest' | 'history' | 'compare'
const scoreHistory = [7200, 7450, 7800, 8100, 8200, 8420, 8890]
export function Benchmark() {
  const [activeTab, setActiveTab] = useState<Tab>('latest')
  const tabs: {
    id: Tab
    label: string
  }[] = [
    {
      id: 'latest',
      label: 'Latest Result',
    },
    {
      id: 'history',
      label: 'History',
    },
    {
      id: 'compare',
      label: 'Compare',
    },
  ]
  return (
    <div className="space-y-5">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
          Benchmark
        </h1>
        <button className="px-4 py-1.5 bg-accent text-white text-sm font-medium rounded-md hover:bg-accent-hover transition-colors flex items-center gap-2">
          <PlayIcon className="w-4 h-4" /> Run Benchmark
        </button>
      </div>

      <div className="flex items-center gap-0 border-b border-border-subtle">
        {tabs.map((tab) => (
          <button
            key={tab.id}
            onClick={() => setActiveTab(tab.id)}
            className={`px-4 py-2 text-sm transition-colors relative ${activeTab === tab.id ? 'text-txt-primary' : 'text-txt-tertiary hover:text-txt-secondary'}`}
          >
            {tab.label}
            {activeTab === tab.id && (
              <span className="absolute bottom-0 left-0 right-0 h-0.5 bg-accent rounded-t" />
            )}
          </button>
        ))}
      </div>

      {activeTab === 'latest' && (
        <div className="space-y-5">
          {/* Metadata */}
          <div className="flex items-center gap-4 text-sm text-txt-tertiary">
            <span>Benchmark #47</span>
            <span>·</span>
            <span>Jan 15, 2025 14:45</span>
            <span>·</span>
            <span>Duration: 4m 32s</span>
            <span>·</span>
            <span>Tweaks active: 14</span>
          </div>

          {/* Verdict Card */}
          <Card elevated className="p-6">
            <div className="text-[11px] font-semibold text-txt-tertiary uppercase tracking-wider mb-4">
              Verdict
            </div>
            <div className="flex items-end justify-between">
              <div>
                <div className="text-4xl font-bold font-mono text-txt-primary tracking-tight">
                  8,890
                </div>
                <div className="text-sm text-txt-tertiary mt-1">
                  Previous: 8,420
                </div>
              </div>
              <div className="text-right">
                <div className="flex items-center gap-2 text-success">
                  <TrendingUpIcon className="w-5 h-5" />
                  <span className="text-2xl font-bold">+5.6%</span>
                </div>
                <div className="text-sm font-medium text-success mt-1">
                  Measurable Improvement
                </div>
              </div>
            </div>
          </Card>

          {/* Sub-scores */}
          <div className="grid grid-cols-3 gap-4">
            {[
              {
                label: 'CPU Score',
                value: '4,210',
                delta: '+3.2%',
                positive: true,
              },
              {
                label: 'GPU Score',
                value: '3,890',
                delta: '+8.1%',
                positive: true,
              },
              {
                label: 'Memory Score',
                value: '790',
                delta: '+2.4%',
                positive: true,
              },
            ].map((score) => (
              <Card key={score.label} className="p-4">
                <div className="text-[11px] text-txt-tertiary mb-2">
                  {score.label}
                </div>
                <div className="flex items-end justify-between">
                  <span className="text-xl font-bold font-mono text-txt-primary">
                    {score.value}
                  </span>
                  <span
                    className={`text-sm font-semibold ${score.positive ? 'text-success' : 'text-error'}`}
                  >
                    {score.delta}
                  </span>
                </div>
              </Card>
            ))}
          </div>

          {/* Tweak Attribution */}
          <Card className="p-5">
            <h3 className="text-xs font-semibold text-txt-tertiary uppercase tracking-wider mb-4">
              Tweak Attribution
            </h3>
            <div className="space-y-3">
              {[
                {
                  tweak: 'Power Plan change',
                  impact: '+2.1% CPU',
                  positive: true,
                },
                {
                  tweak: "Nagle's Algorithm disable",
                  impact: '-8ms latency',
                  positive: true,
                },
                {
                  tweak: 'GPU Shader Cache enable',
                  impact: '+4.2% GPU',
                  positive: true,
                },
                {
                  tweak: 'RSS enable',
                  impact: '+1.8% throughput',
                  positive: true,
                },
                {
                  tweak: 'Search Indexing disable',
                  impact: '-12% disk I/O',
                  positive: true,
                },
              ].map((attr) => (
                <div
                  key={attr.tweak}
                  className="flex items-center justify-between py-2 border-b border-border-subtle last:border-0"
                >
                  <span className="text-sm text-txt-secondary">
                    {attr.tweak}
                  </span>
                  <span
                    className={`text-sm font-mono font-medium ${attr.positive ? 'text-success' : 'text-error'}`}
                  >
                    {attr.impact}
                  </span>
                </div>
              ))}
            </div>
          </Card>
        </div>
      )}

      {activeTab === 'history' && (
        <div className="space-y-5">
          {/* Score Trend */}
          <Card className="p-5">
            <div className="flex items-center justify-between mb-3">
              <h3 className="text-xs font-semibold text-txt-tertiary uppercase tracking-wider">
                Score Trend
              </h3>
            </div>
            <div className="h-24">
              <MiniChart
                data={scoreHistory}
                color="#6366f1"
                width={720}
                height={96}
              />
            </div>
          </Card>

          {/* History Table */}
          <Card className="overflow-hidden">
            <table className="w-full">
              <thead>
                <tr className="bg-surface-sunken">
                  <th className="text-left text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                    Run
                  </th>
                  <th className="text-left text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                    Date
                  </th>
                  <th className="text-right text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                    Score
                  </th>
                  <th className="text-right text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                    Delta
                  </th>
                  <th className="text-right text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                    Tweaks
                  </th>
                  <th className="text-right text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                    Duration
                  </th>
                </tr>
              </thead>
              <tbody>
                {[
                  {
                    run: 47,
                    date: 'Jan 15, 14:45',
                    score: '8,890',
                    delta: '+5.6%',
                    positive: true,
                    tweaks: 14,
                    duration: '4m 32s',
                  },
                  {
                    run: 46,
                    date: 'Jan 14, 22:00',
                    score: '8,420',
                    delta: '+2.7%',
                    positive: true,
                    tweaks: 11,
                    duration: '4m 28s',
                  },
                  {
                    run: 45,
                    date: 'Jan 13, 18:30',
                    score: '8,200',
                    delta: '+1.2%',
                    positive: true,
                    tweaks: 10,
                    duration: '4m 35s',
                  },
                  {
                    run: 44,
                    date: 'Jan 12, 15:00',
                    score: '8,100',
                    delta: '+3.8%',
                    positive: true,
                    tweaks: 8,
                    duration: '4m 30s',
                  },
                  {
                    run: 43,
                    date: 'Jan 11, 20:00',
                    score: '7,800',
                    delta: '+4.7%',
                    positive: true,
                    tweaks: 5,
                    duration: '4m 25s',
                  },
                  {
                    run: 42,
                    date: 'Jan 10, 14:00',
                    score: '7,450',
                    delta: '+3.5%',
                    positive: true,
                    tweaks: 3,
                    duration: '4m 22s',
                  },
                  {
                    run: 41,
                    date: 'Jan 9, 12:00',
                    score: '7,200',
                    delta: '—',
                    positive: false,
                    tweaks: 0,
                    duration: '4m 20s',
                  },
                ].map((row) => (
                  <tr
                    key={row.run}
                    className="border-t border-border-subtle hover:bg-surface-hover transition-colors cursor-pointer"
                  >
                    <td className="px-4 py-3 text-sm text-txt-primary font-medium">
                      #{row.run}
                    </td>
                    <td className="px-4 py-3 text-sm text-txt-secondary">
                      {row.date}
                    </td>
                    <td className="px-4 py-3 text-sm font-mono text-txt-primary text-right">
                      {row.score}
                    </td>
                    <td
                      className={`px-4 py-3 text-sm font-mono text-right ${row.positive ? 'text-success' : 'text-txt-tertiary'}`}
                    >
                      {row.delta}
                    </td>
                    <td className="px-4 py-3 text-sm text-txt-tertiary text-right">
                      {row.tweaks}
                    </td>
                    <td className="px-4 py-3 text-sm text-txt-tertiary text-right font-mono">
                      {row.duration}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </Card>
        </div>
      )}

      {activeTab === 'compare' && (
        <div className="space-y-5">
          <div className="grid grid-cols-2 gap-4">
            <div>
              <label className="text-[11px] text-txt-tertiary uppercase tracking-wider mb-1.5 block">
                Run A
              </label>
              <select className="w-full px-3 py-1.5 text-sm bg-surface-sunken border border-border-default rounded-md text-txt-primary focus:outline-none focus:border-accent">
                <option>#47 — Jan 15 (8,890)</option>
                <option>#46 — Jan 14 (8,420)</option>
                <option>#45 — Jan 13 (8,200)</option>
              </select>
            </div>
            <div>
              <label className="text-[11px] text-txt-tertiary uppercase tracking-wider mb-1.5 block">
                Run B
              </label>
              <select className="w-full px-3 py-1.5 text-sm bg-surface-sunken border border-border-default rounded-md text-txt-primary focus:outline-none focus:border-accent">
                <option>#41 — Jan 9 (7,200)</option>
                <option>#42 — Jan 10 (7,450)</option>
                <option>#43 — Jan 11 (7,800)</option>
              </select>
            </div>
          </div>

          <Card className="overflow-hidden">
            <table className="w-full">
              <thead>
                <tr className="bg-surface-sunken">
                  <th className="text-left text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                    Metric
                  </th>
                  <th className="text-right text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                    Run #47
                  </th>
                  <th className="text-right text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                    Run #41
                  </th>
                  <th className="text-right text-[11px] font-medium text-txt-tertiary uppercase tracking-wider px-4 py-2.5">
                    Delta
                  </th>
                </tr>
              </thead>
              <tbody>
                {[
                  {
                    metric: 'Overall Score',
                    a: '8,890',
                    b: '7,200',
                    delta: '+23.5%',
                  },
                  {
                    metric: 'CPU Score',
                    a: '4,210',
                    b: '3,500',
                    delta: '+20.3%',
                  },
                  {
                    metric: 'GPU Score',
                    a: '3,890',
                    b: '3,100',
                    delta: '+25.5%',
                  },
                  {
                    metric: 'Memory Score',
                    a: '790',
                    b: '600',
                    delta: '+31.7%',
                  },
                ].map((row) => (
                  <tr
                    key={row.metric}
                    className="border-t border-border-subtle"
                  >
                    <td className="px-4 py-3 text-sm text-txt-secondary">
                      {row.metric}
                    </td>
                    <td className="px-4 py-3 text-sm font-mono text-txt-primary text-right">
                      {row.a}
                    </td>
                    <td className="px-4 py-3 text-sm font-mono text-txt-tertiary text-right">
                      {row.b}
                    </td>
                    <td className="px-4 py-3 text-sm font-mono text-success font-medium text-right">
                      {row.delta}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </Card>
        </div>
      )}
    </div>
  )
}

```
```pages/Dashboard.tsx
import React from 'react'
import {
  ActivityIcon,
  ShieldCheckIcon,
  ZapIcon,
  ArrowRightIcon,
  CheckCircleIcon,
} from 'lucide-react'
import { Card } from '../components/Card'
import { HealthRing } from '../components/HealthRing'
import { StatusBadge } from '../components/StatusBadge'
interface DashboardProps {
  onNavigate: (page: string) => void
}
export function Dashboard({ onNavigate }: DashboardProps) {
  return (
    <div className="space-y-6">
      {/* Page Header */}
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
          Dashboard
        </h1>
        <StatusBadge status="LIVE" />
      </div>

      {/* Machine State Hero */}
      <Card className="p-6">
        <div className="flex items-center gap-6">
          <HealthRing score={92} size={72} strokeWidth={5} />
          <div className="flex-1 space-y-1">
            <div className="flex items-center gap-2">
              <h2 className="text-lg font-semibold text-txt-primary">
                System Health: Excellent
              </h2>
              <span className="w-2 h-2 rounded-full bg-success animate-pulse-dot" />
            </div>
            <div className="flex items-center gap-4 text-sm text-txt-secondary">
              <span className="flex items-center gap-1.5">
                <ActivityIcon className="w-3.5 h-3.5" />
                Session active · 2h 14m
              </span>
              <span>Last optimization: 12 min ago</span>
            </div>
            <div className="flex items-center gap-1.5 text-sm text-success">
              <ShieldCheckIcon className="w-3.5 h-3.5" />
              Recovery: Ready
            </div>
          </div>
        </div>
      </Card>

      {/* Two Column Layout */}
      <div className="grid grid-cols-5 gap-5">
        {/* Left Column */}
        <div className="col-span-3 space-y-5">
          {/* Top Telemetry */}
          <Card className="p-5">
            <h3 className="text-[15px] font-semibold text-txt-primary mb-4">
              System
            </h3>
            <div className="space-y-3">
              {[
                {
                  label: 'CPU',
                  value: '34%',
                  temp: '62°C',
                  pct: 34,
                  color: 'bg-accent',
                },
                {
                  label: 'GPU',
                  value: '12%',
                  temp: '48°C',
                  pct: 12,
                  color: 'bg-success',
                },
                {
                  label: 'RAM',
                  value: '67%',
                  temp: null,
                  pct: 67,
                  color: 'bg-warning',
                },
                {
                  label: 'Disk',
                  value: '45%',
                  temp: null,
                  pct: 45,
                  color: 'bg-accent',
                },
              ].map((m) => (
                <div key={m.label} className="flex items-center gap-3">
                  <span className="text-xs text-txt-tertiary w-8 font-medium">
                    {m.label}
                  </span>
                  <div className="flex-1 h-1 bg-surface-sunken rounded-full overflow-hidden">
                    <div
                      className={`h-full ${m.color} rounded-full transition-all`}
                      style={{
                        width: `${m.pct}%`,
                      }}
                    />
                  </div>
                  <span className="text-sm font-semibold text-txt-primary w-10 text-right font-mono">
                    {m.value}
                  </span>
                  {m.temp && (
                    <span className="text-[11px] text-txt-tertiary font-mono w-10">
                      {m.temp}
                    </span>
                  )}
                  {!m.temp && <span className="w-10" />}
                </div>
              ))}
            </div>
          </Card>

          {/* Trust Posture */}
          <Card className="p-5">
            <div className="flex items-center justify-between mb-4">
              <h3 className="text-[15px] font-semibold text-txt-primary">
                Trust & Recovery
              </h3>
              <button
                onClick={() => onNavigate('trust')}
                className="text-xs text-accent hover:text-accent-hover transition-colors flex items-center gap-1"
              >
                Trust Center <ArrowRightIcon className="w-3 h-3" />
              </button>
            </div>
            <div className="grid grid-cols-2 gap-3">
              {[
                {
                  label: 'Rollback',
                  value: 'Ready',
                  icon: CheckCircleIcon,
                  color: 'text-success',
                },
                {
                  label: 'Snapshot',
                  value: '2h ago',
                  icon: ShieldCheckIcon,
                  color: 'text-txt-secondary',
                },
                {
                  label: 'Active tweaks',
                  value: '14',
                  icon: ZapIcon,
                  color: 'text-accent',
                },
                {
                  label: 'Reverted',
                  value: '2',
                  icon: ActivityIcon,
                  color: 'text-txt-tertiary',
                },
              ].map((item) => (
                <div
                  key={item.label}
                  className="flex items-center gap-2.5 p-2.5 rounded-md bg-surface-sunken"
                >
                  <item.icon
                    className={`w-4 h-4 ${item.color} flex-shrink-0`}
                  />
                  <div>
                    <div className="text-[11px] text-txt-tertiary">
                      {item.label}
                    </div>
                    <div className="text-sm font-medium text-txt-primary">
                      {item.value}
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </Card>
        </div>

        {/* Right Column */}
        <div className="col-span-2">
          <Card className="p-5 h-full">
            <div className="flex items-center justify-between mb-4">
              <h3 className="text-[15px] font-semibold text-txt-primary">
                Recommendations
              </h3>
              <span className="text-[10px] font-semibold uppercase tracking-wider text-accent bg-accent-muted px-2 py-0.5 rounded">
                3 available
              </span>
            </div>
            <div className="space-y-3">
              {[
                {
                  text: 'Disable Windows Search Indexing',
                  risk: null,
                  premium: false,
                },
                {
                  text: 'Adjust power plan for gaming',
                  risk: 'Low',
                  premium: false,
                },
                {
                  text: 'Advanced CPU parking control',
                  risk: 'Medium',
                  premium: true,
                },
              ].map((rec, i) => (
                <div
                  key={i}
                  className="flex items-start gap-2.5 p-3 rounded-md bg-surface-sunken hover:bg-surface-hover transition-colors cursor-pointer group"
                >
                  <span
                    className={`w-1.5 h-1.5 rounded-full mt-1.5 flex-shrink-0 ${rec.premium ? 'bg-premium' : 'bg-accent'}`}
                  />
                  <div className="flex-1 min-w-0">
                    <div className="text-sm text-txt-primary group-hover:text-white transition-colors">
                      {rec.text}
                    </div>
                    <div className="flex items-center gap-2 mt-1">
                      {rec.risk && (
                        <span className="text-[10px] text-txt-tertiary">
                          Risk: {rec.risk}
                        </span>
                      )}
                      {rec.premium && <StatusBadge status="PREMIUM" />}
                    </div>
                  </div>
                </div>
              ))}
            </div>
            <button
              onClick={() => onNavigate('optimizations')}
              className="mt-4 text-xs text-accent hover:text-accent-hover transition-colors flex items-center gap-1"
            >
              View all optimizations <ArrowRightIcon className="w-3 h-3" />
            </button>
          </Card>
        </div>
      </div>
    </div>
  )
}

```
```pages/GameProfiles.tsx
import React, { useState } from 'react'
import { PlusIcon, UploadIcon, Gamepad2Icon, StarIcon } from 'lucide-react'
import { Card } from '../components/Card'
import { StatusBadge } from '../components/StatusBadge'
type Tab = 'my' | 'recommended' | 'community'
interface Profile {
  id: string
  name: string
  tweaks: number
  lastUsed: string
  stability: 'Excellent' | 'Good' | 'Fair'
  scoreDelta: string
  active: boolean
  premium?: boolean
  icon?: string
}
const myProfiles: Profile[] = [
  {
    id: '1',
    name: 'Cyberpunk 2077',
    tweaks: 12,
    lastUsed: '2h ago',
    stability: 'Excellent',
    scoreDelta: '+8.2%',
    active: true,
    icon: '🎮',
  },
  {
    id: '2',
    name: 'Valorant',
    tweaks: 8,
    lastUsed: '1d ago',
    stability: 'Good',
    scoreDelta: '+3.1%',
    active: false,
    icon: '🎯',
  },
  {
    id: '3',
    name: 'Elden Ring',
    tweaks: 10,
    lastUsed: '3d ago',
    stability: 'Excellent',
    scoreDelta: '+5.4%',
    active: false,
    icon: '⚔️',
  },
  {
    id: '4',
    name: 'Flight Simulator 2024',
    tweaks: 18,
    lastUsed: '—',
    stability: 'Good',
    scoreDelta: '+12.1%',
    active: false,
    premium: true,
    icon: '✈️',
  },
]
const stabilityColor = (s: string) =>
  s === 'Excellent'
    ? 'text-success'
    : s === 'Good'
      ? 'text-accent'
      : 'text-warning'
export function GameProfiles() {
  const [activeTab, setActiveTab] = useState<Tab>('my')
  const tabs: {
    id: Tab
    label: string
  }[] = [
    {
      id: 'my',
      label: 'My Profiles',
    },
    {
      id: 'recommended',
      label: 'Recommended',
    },
    {
      id: 'community',
      label: 'Community',
    },
  ]
  return (
    <div className="space-y-5">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
          Game Profiles
        </h1>
        <div className="flex items-center gap-2">
          <button className="px-3 py-1.5 text-xs border border-border-default text-txt-secondary rounded-md hover:bg-surface-hover transition-colors flex items-center gap-1.5">
            <UploadIcon className="w-3.5 h-3.5" /> Import
          </button>
          <button className="px-4 py-1.5 bg-accent text-white text-sm font-medium rounded-md hover:bg-accent-hover transition-colors flex items-center gap-2">
            <PlusIcon className="w-4 h-4" /> Create Profile
          </button>
        </div>
      </div>

      <div className="flex items-center gap-0 border-b border-border-subtle">
        {tabs.map((tab) => (
          <button
            key={tab.id}
            onClick={() => setActiveTab(tab.id)}
            className={`px-4 py-2 text-sm transition-colors relative ${activeTab === tab.id ? 'text-txt-primary' : 'text-txt-tertiary hover:text-txt-secondary'}`}
          >
            {tab.label}
            {tab.id === 'community' && (
              <span className="ml-1.5">
                <StatusBadge status="PREMIUM" />
              </span>
            )}
            {activeTab === tab.id && (
              <span className="absolute bottom-0 left-0 right-0 h-0.5 bg-accent rounded-t" />
            )}
          </button>
        ))}
      </div>

      {activeTab === 'my' && (
        <div className="space-y-3">
          {myProfiles.map((profile) => (
            <Card
              key={profile.id}
              className={`p-5 ${profile.premium ? 'opacity-75' : ''}`}
              statusColor={profile.active ? '#34d399' : undefined}
            >
              <div className="flex items-center gap-4">
                <div className="w-12 h-12 rounded-lg bg-surface-sunken flex items-center justify-center text-2xl flex-shrink-0">
                  {profile.icon}
                </div>
                <div className="flex-1 min-w-0">
                  <div className="flex items-center gap-2 mb-1">
                    <h3 className="text-sm font-semibold text-txt-primary">
                      {profile.name}
                    </h3>
                    {profile.active && <StatusBadge status="ACTIVE" />}
                    {profile.premium && <StatusBadge status="PREMIUM" />}
                  </div>
                  <div className="flex items-center gap-4 text-xs text-txt-tertiary">
                    <span>{profile.tweaks} tweaks</span>
                    <span>Last used: {profile.lastUsed}</span>
                    <span>
                      Stability:{' '}
                      <span className={stabilityColor(profile.stability)}>
                        {profile.stability}
                      </span>
                    </span>
                    <span>
                      Score:{' '}
                      <span className="text-success font-medium">
                        {profile.scoreDelta}
                      </span>
                    </span>
                  </div>
                </div>
                <div className="flex items-center gap-2 flex-shrink-0">
                  {profile.premium ? (
                    <button className="px-3 py-1.5 text-xs bg-premium-muted text-premium rounded-md hover:bg-premium/20 transition-colors">
                      Unlock Pro
                    </button>
                  ) : (
                    <>
                      <button className="px-3 py-1 text-xs text-txt-tertiary hover:text-txt-secondary transition-colors">
                        Edit
                      </button>
                      <button className="px-3 py-1 text-xs text-txt-tertiary hover:text-txt-secondary transition-colors">
                        Benchmark
                      </button>
                      {profile.active ? (
                        <button className="px-3 py-1.5 text-xs border border-border-default text-txt-primary rounded-md hover:bg-surface-hover transition-colors">
                          Deactivate
                        </button>
                      ) : (
                        <button className="px-3 py-1.5 text-xs border border-accent text-accent rounded-md hover:bg-accent-muted transition-colors">
                          Activate
                        </button>
                      )}
                    </>
                  )}
                </div>
              </div>
            </Card>
          ))}
        </div>
      )}

      {activeTab === 'recommended' && (
        <div className="space-y-3">
          {[
            {
              name: 'Counter-Strike 2',
              tweaks: 9,
              improvement: '3–6%',
              compat: 'All systems',
              icon: '🔫',
            },
            {
              name: 'Fortnite',
              tweaks: 7,
              improvement: '2–5%',
              compat: 'All systems',
              icon: '🏗️',
            },
            {
              name: 'Hogwarts Legacy',
              tweaks: 14,
              improvement: '8–15%',
              compat: 'RTX 30/40 series',
              icon: '🧙',
            },
            {
              name: 'Starfield',
              tweaks: 16,
              improvement: '10–18%',
              compat: 'RTX 40 series',
              premium: true,
              icon: '🚀',
            },
          ].map((rec) => (
            <Card
              key={rec.name}
              className={`p-5 ${rec.premium ? 'opacity-75' : ''}`}
            >
              <div className="flex items-center gap-4">
                <div className="w-12 h-12 rounded-lg bg-surface-sunken flex items-center justify-center text-2xl flex-shrink-0">
                  {rec.icon}
                </div>
                <div className="flex-1">
                  <div className="flex items-center gap-2 mb-1">
                    <h3 className="text-sm font-semibold text-txt-primary">
                      {rec.name}
                    </h3>
                    {rec.premium && <StatusBadge status="PREMIUM" />}
                  </div>
                  <div className="flex items-center gap-4 text-xs text-txt-tertiary">
                    <span>{rec.tweaks} tweaks</span>
                    <span>
                      Expected:{' '}
                      <span className="text-success">{rec.improvement}</span>
                    </span>
                    <span>{rec.compat}</span>
                  </div>
                </div>
                {rec.premium ? (
                  <button className="px-3 py-1.5 text-xs bg-premium-muted text-premium rounded-md">
                    Unlock Pro
                  </button>
                ) : (
                  <button className="px-3 py-1.5 text-xs border border-accent text-accent rounded-md hover:bg-accent-muted transition-colors">
                    Apply
                  </button>
                )}
              </div>
            </Card>
          ))}
        </div>
      )}

      {activeTab === 'community' && (
        <div className="flex flex-col items-center justify-center py-16 text-center">
          <div className="w-12 h-12 rounded-full bg-premium-muted flex items-center justify-center mb-4">
            <Gamepad2Icon className="w-6 h-6 text-premium" />
          </div>
          <h3 className="text-[15px] font-semibold text-txt-secondary mb-2">
            Community Profiles
          </h3>
          <p className="text-sm text-txt-tertiary mb-4 max-w-sm">
            Browse and import optimization profiles shared by the PulseBoost
            community. Available with Pro.
          </p>
          <button className="px-4 py-1.5 bg-premium-muted text-premium text-sm font-medium rounded-md hover:bg-premium/20 transition-colors">
            Upgrade to Pro
          </button>
        </div>
      )}
    </div>
  )
}

```
```pages/GPU.tsx
import React, { useState } from 'react'
import { Card } from '../components/Card'
import { MiniChart } from '../components/MiniChart'
import { StatusBadge } from '../components/StatusBadge'
import { InfoIcon, ExternalLinkIcon } from 'lucide-react'
const gpuLoadData = [
  45, 48, 52, 50, 55, 60, 58, 62, 65, 60, 55, 50, 48, 52, 55, 58, 60, 62, 58,
  55,
]
const tempData = [
  58, 59, 60, 61, 62, 63, 62, 63, 64, 63, 62, 61, 60, 61, 62, 63, 62, 61, 62,
  62,
]
const clockData = [
  1800, 1850, 1890, 1920, 1920, 1905, 1920, 1920, 1890, 1920, 1920, 1905, 1920,
  1920, 1890, 1920, 1920, 1905, 1920, 1920,
]
const powerData = [
  120, 125, 135, 140, 142, 138, 142, 145, 140, 142, 142, 138, 142, 145, 140,
  142, 142, 138, 142, 142,
]
type Tab = 'telemetry' | 'optimizations' | 'driver' | 'advisory'
export function GPU() {
  const [activeTab, setActiveTab] = useState<Tab>('telemetry')
  const tabs: {
    id: Tab
    label: string
  }[] = [
    {
      id: 'telemetry',
      label: 'Telemetry',
    },
    {
      id: 'optimizations',
      label: 'Optimizations',
    },
    {
      id: 'driver',
      label: 'Driver',
    },
    {
      id: 'advisory',
      label: 'Advisory',
    },
  ]
  return (
    <div className="space-y-5">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
          GPU
        </h1>
        <div className="flex items-center gap-2 px-3 py-1.5 rounded-md bg-success-muted border border-success/20">
          <span className="text-xs font-semibold text-success">
            NVIDIA RTX 4070
          </span>
        </div>
      </div>

      <div className="flex items-center gap-0 border-b border-border-subtle">
        {tabs.map((tab) => (
          <button
            key={tab.id}
            onClick={() => setActiveTab(tab.id)}
            className={`px-4 py-2 text-sm transition-colors relative ${activeTab === tab.id ? 'text-txt-primary' : 'text-txt-tertiary hover:text-txt-secondary'}`}
          >
            {tab.label}
            {activeTab === tab.id && (
              <span className="absolute bottom-0 left-0 right-0 h-0.5 bg-accent rounded-t" />
            )}
          </button>
        ))}
      </div>

      {activeTab === 'telemetry' && (
        <div className="space-y-5">
          {/* Metric Cards */}
          <div className="grid grid-cols-4 gap-4">
            {[
              {
                label: 'Core Clock',
                value: '1,920 MHz',
                data: clockData,
                color: '#6366f1',
              },
              {
                label: 'Memory',
                value: '8.2 / 12 GB',
                data: gpuLoadData,
                color: '#60a5fa',
              },
              {
                label: 'Temperature',
                value: '62°C',
                data: tempData,
                color: '#fbbf24',
              },
              {
                label: 'Power Draw',
                value: '142W',
                data: powerData,
                color: '#34d399',
              },
            ].map((m) => (
              <Card key={m.label} className="p-4">
                <div className="text-[11px] text-txt-tertiary mb-1">
                  {m.label}
                </div>
                <div className="flex items-end justify-between">
                  <span className="text-lg font-semibold font-mono text-txt-primary">
                    {m.value}
                  </span>
                  <MiniChart
                    data={m.data}
                    color={m.color}
                    width={60}
                    height={24}
                  />
                </div>
              </Card>
            ))}
          </div>

          {/* GPU Load Chart */}
          <Card className="p-5">
            <div className="flex items-center justify-between mb-3">
              <h3 className="text-xs font-semibold text-txt-tertiary uppercase tracking-wider">
                GPU Load (30 min)
              </h3>
              <span className="text-sm font-mono text-txt-primary">55%</span>
            </div>
            <div className="h-32 flex items-end">
              <MiniChart
                data={gpuLoadData}
                color="#6366f1"
                width={720}
                height={128}
              />
            </div>
            <div className="flex justify-between mt-2 text-[10px] text-txt-disabled">
              <span>30m ago</span>
              <span>Now</span>
            </div>
          </Card>

          {/* Fan & Thermal */}
          <Card className="p-5">
            <h3 className="text-xs font-semibold text-txt-tertiary uppercase tracking-wider mb-4">
              Fan Speed & Thermal
            </h3>
            <div className="grid grid-cols-2 gap-6">
              <div className="space-y-3">
                {[
                  {
                    label: 'Fan 1',
                    value: '1,240 RPM',
                    pct: '45%',
                  },
                  {
                    label: 'Fan 2',
                    value: '1,180 RPM',
                    pct: '43%',
                  },
                ].map((fan) => (
                  <div
                    key={fan.label}
                    className="flex items-center justify-between"
                  >
                    <span className="text-sm text-txt-secondary">
                      {fan.label}
                    </span>
                    <div className="text-right">
                      <span className="text-sm font-mono text-txt-primary">
                        {fan.value}
                      </span>
                      <span className="text-xs text-txt-tertiary ml-2">
                        ({fan.pct})
                      </span>
                    </div>
                  </div>
                ))}
              </div>
              <div className="space-y-3">
                <div className="flex items-center justify-between">
                  <span className="text-sm text-txt-secondary">
                    Target Temp
                  </span>
                  <span className="text-sm font-mono text-txt-primary">
                    65°C
                  </span>
                </div>
                <div className="flex items-center justify-between">
                  <span className="text-sm text-txt-secondary">Throttle</span>
                  <span className="text-sm font-medium text-success">None</span>
                </div>
              </div>
            </div>
          </Card>
        </div>
      )}

      {activeTab === 'optimizations' && (
        <div className="space-y-3">
          {[
            {
              name: 'Shader Cache Optimization',
              desc: 'Configure shader cache size and location',
              badges: ['VALIDATED' as const, 'ACTIVE' as const],
              risk: 'Low',
            },
            {
              name: 'Power Management Mode',
              desc: 'Set to Prefer Maximum Performance',
              badges: ['VALIDATED' as const],
              risk: 'Low',
            },
            {
              name: 'Threaded Optimization',
              desc: 'Enable multi-threaded OpenGL rendering',
              badges: ['VALIDATED' as const, 'HARDWARE_SPECIFIC' as const],
              risk: 'Low',
            },
          ].map((opt) => (
            <Card key={opt.name} className="p-4">
              <div className="flex items-center justify-between">
                <div>
                  <div className="flex items-center gap-2 mb-1">
                    <h4 className="text-sm font-semibold text-txt-primary">
                      {opt.name}
                    </h4>
                    {opt.badges.map((b) => (
                      <StatusBadge key={b} status={b} />
                    ))}
                  </div>
                  <p className="text-xs text-txt-secondary">{opt.desc}</p>
                </div>
                <button className="px-3 py-1 text-xs border border-border-default text-txt-primary rounded-md hover:bg-surface-hover transition-colors">
                  {opt.badges.includes('ACTIVE') ? 'Revert' : 'Apply'}
                </button>
              </div>
            </Card>
          ))}
        </div>
      )}

      {activeTab === 'driver' && (
        <div className="space-y-5">
          <Card className="p-5">
            <h3 className="text-[15px] font-semibold text-txt-primary mb-4">
              Current Driver
            </h3>
            <div className="space-y-2 text-sm">
              <div className="flex justify-between">
                <span className="text-txt-tertiary">Version</span>
                <span className="font-mono text-txt-primary">551.86</span>
              </div>
              <div className="flex justify-between">
                <span className="text-txt-tertiary">Date</span>
                <span className="text-txt-primary">January 8, 2025</span>
              </div>
              <div className="flex justify-between">
                <span className="text-txt-tertiary">Type</span>
                <span className="text-txt-primary">Game Ready Driver</span>
              </div>
              <div className="flex justify-between">
                <span className="text-txt-tertiary">Status</span>
                <span className="text-success font-medium">Up to date</span>
              </div>
            </div>
          </Card>
          <Card className="p-4 border-border-subtle">
            <div className="flex items-start gap-3">
              <InfoIcon className="w-4 h-4 text-txt-tertiary mt-0.5 flex-shrink-0" />
              <div>
                <p className="text-sm text-txt-secondary">
                  PulseBoost does not install or modify GPU drivers. Visit the
                  NVIDIA website for driver downloads.
                </p>
                <a
                  href="#"
                  className="text-xs text-accent hover:text-accent-hover mt-1 inline-flex items-center gap-1"
                >
                  NVIDIA Driver Downloads{' '}
                  <ExternalLinkIcon className="w-3 h-3" />
                </a>
              </div>
            </div>
          </Card>
        </div>
      )}

      {activeTab === 'advisory' && (
        <div className="space-y-5">
          <Card className="p-4 border-info/20 bg-info-muted/50">
            <div className="flex items-start gap-3">
              <InfoIcon className="w-4 h-4 text-info mt-0.5 flex-shrink-0" />
              <p className="text-sm text-info/80">
                These settings require BIOS changes and cannot be modified by
                PulseBoost. They are shown for informational purposes only.
              </p>
            </div>
          </Card>
          {[
            {
              name: 'Resizable BAR',
              status: 'Enabled',
              recommendation: 'Correctly configured',
              ok: true,
            },
            {
              name: 'Above 4G Decoding',
              status: 'Enabled',
              recommendation: 'Required for Resizable BAR',
              ok: true,
            },
            {
              name: 'PCIe Generation',
              status: 'Gen 4',
              recommendation: 'Optimal for RTX 4070',
              ok: true,
            },
          ].map((item) => (
            <Card key={item.name} className="p-4">
              <div className="flex items-center justify-between">
                <div>
                  <div className="flex items-center gap-2 mb-1">
                    <h4 className="text-sm font-semibold text-txt-primary">
                      {item.name}
                    </h4>
                    <span className="text-[10px] font-medium px-2 py-0.5 rounded bg-surface-hover text-txt-tertiary uppercase tracking-wider">
                      BIOS
                    </span>
                  </div>
                  <p className="text-xs text-txt-tertiary">
                    {item.recommendation}
                  </p>
                </div>
                <span
                  className={`text-sm font-mono ${item.ok ? 'text-success' : 'text-warning'}`}
                >
                  {item.status}
                </span>
              </div>
            </Card>
          ))}
        </div>
      )}
    </div>
  )
}

```
```pages/Network.tsx
import React, { useState } from 'react'
import {
  WifiIcon,
  ActivityIcon,
  ArrowDownIcon,
  ArrowUpIcon,
  CheckCircleIcon,
  AlertTriangleIcon,
  InfoIcon,
} from 'lucide-react'
import { Card } from '../components/Card'
import { MiniChart } from '../components/MiniChart'
import { StatusBadge } from '../components/StatusBadge'
const latencyData = [
  14, 15, 13, 16, 14, 12, 15, 14, 13, 15, 14, 16, 13, 14, 15, 14, 12, 13, 14,
  15,
]
const throughputData = [
  380, 412, 395, 420, 410, 405, 415, 412, 408, 420, 418, 412, 405, 410, 415,
  420, 412, 408, 415, 412,
]
type Tab = 'overview' | 'diagnostics' | 'qos' | 'adapters'
export function Network() {
  const [activeTab, setActiveTab] = useState<Tab>('overview')
  const tabs: {
    id: Tab
    label: string
  }[] = [
    {
      id: 'overview',
      label: 'Overview',
    },
    {
      id: 'diagnostics',
      label: 'Diagnostics',
    },
    {
      id: 'qos',
      label: 'QoS',
    },
    {
      id: 'adapters',
      label: 'Adapters',
    },
  ]
  return (
    <div className="space-y-5">
      <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
        Network
      </h1>

      {/* Tabs */}
      <div className="flex items-center gap-0 border-b border-border-subtle">
        {tabs.map((tab) => (
          <button
            key={tab.id}
            onClick={() => setActiveTab(tab.id)}
            className={`px-4 py-2 text-sm transition-colors relative ${activeTab === tab.id ? 'text-txt-primary' : 'text-txt-tertiary hover:text-txt-secondary'}`}
          >
            {tab.label}
            {activeTab === tab.id && (
              <span className="absolute bottom-0 left-0 right-0 h-0.5 bg-accent rounded-t" />
            )}
          </button>
        ))}
      </div>

      {activeTab === 'overview' && (
        <div className="space-y-5">
          {/* Connection Status */}
          <Card className="p-5">
            <div className="flex items-center gap-3 mb-4">
              <div className="w-10 h-10 rounded-lg bg-success-muted flex items-center justify-center">
                <WifiIcon className="w-5 h-5 text-success" />
              </div>
              <div>
                <h3 className="text-sm font-semibold text-txt-primary">
                  Intel I225-V · Ethernet · 1 Gbps
                </h3>
                <p className="text-xs text-success">Connected</p>
              </div>
            </div>
            <div className="grid grid-cols-4 gap-4">
              {[
                {
                  label: 'Latency',
                  value: '14ms',
                  color: 'text-success',
                },
                {
                  label: 'Jitter',
                  value: '2ms',
                  color: 'text-success',
                },
                {
                  label: 'Packet Loss',
                  value: '0.0%',
                  color: 'text-success',
                },
                {
                  label: 'Bandwidth',
                  value: '412 / 38 Mbps',
                  color: 'text-txt-primary',
                },
              ].map((m) => (
                <div key={m.label} className="text-center">
                  <div className="text-[11px] text-txt-tertiary mb-1">
                    {m.label}
                  </div>
                  <div className={`text-lg font-semibold font-mono ${m.color}`}>
                    {m.value}
                  </div>
                </div>
              ))}
            </div>
          </Card>

          {/* Charts */}
          <div className="grid grid-cols-2 gap-5">
            <Card className="p-5">
              <div className="flex items-center justify-between mb-3">
                <h3 className="text-xs font-semibold text-txt-tertiary uppercase tracking-wider">
                  Latency (30 min)
                </h3>
                <span className="text-sm font-mono text-txt-primary">14ms</span>
              </div>
              <div className="h-24 flex items-end">
                <MiniChart
                  data={latencyData}
                  color="#34d399"
                  width={480}
                  height={96}
                />
              </div>
              <div className="flex justify-between mt-2 text-[10px] text-txt-disabled">
                <span>30m ago</span>
                <span>Now</span>
              </div>
            </Card>
            <Card className="p-5">
              <div className="flex items-center justify-between mb-3">
                <h3 className="text-xs font-semibold text-txt-tertiary uppercase tracking-wider">
                  Throughput (30 min)
                </h3>
                <div className="flex items-center gap-3 text-sm font-mono">
                  <span className="flex items-center gap-1 text-accent">
                    <ArrowDownIcon className="w-3 h-3" />
                    412
                  </span>
                  <span className="flex items-center gap-1 text-txt-tertiary">
                    <ArrowUpIcon className="w-3 h-3" />
                    38
                  </span>
                </div>
              </div>
              <div className="h-24 flex items-end">
                <MiniChart
                  data={throughputData}
                  color="#6366f1"
                  width={480}
                  height={96}
                />
              </div>
              <div className="flex justify-between mt-2 text-[10px] text-txt-disabled">
                <span>30m ago</span>
                <span>Now</span>
              </div>
            </Card>
          </div>

          {/* Recommendations */}
          <Card className="p-5">
            <h3 className="text-xs font-semibold text-txt-tertiary uppercase tracking-wider mb-3">
              Recommendations
            </h3>
            <div className="space-y-2">
              {[
                {
                  text: "Disable Nagle's Algorithm",
                  desc: 'Reduce micro-latency for real-time gaming traffic',
                  badges: ['VALIDATED' as const],
                },
                {
                  text: 'Enable RSS',
                  desc: 'Improve multi-core packet processing for higher throughput',
                  badges: ['VALIDATED' as const, 'HARDWARE_SPECIFIC' as const],
                },
              ].map((rec, i) => (
                <div
                  key={i}
                  className="flex items-center gap-3 p-3 rounded-md bg-surface-sunken"
                >
                  <span className="w-1.5 h-1.5 rounded-full bg-accent flex-shrink-0" />
                  <div className="flex-1 min-w-0">
                    <div className="text-sm text-txt-primary">{rec.text}</div>
                    <div className="text-xs text-txt-tertiary">{rec.desc}</div>
                  </div>
                  <div className="flex gap-1.5">
                    {rec.badges.map((b) => (
                      <StatusBadge key={b} status={b} />
                    ))}
                  </div>
                </div>
              ))}
            </div>
          </Card>
        </div>
      )}

      {activeTab === 'diagnostics' && (
        <div className="space-y-5">
          <div className="flex items-center justify-between">
            <p className="text-sm text-txt-secondary">
              Run a comprehensive network diagnostic to identify bottlenecks and
              configuration issues.
            </p>
            <button className="px-4 py-1.5 bg-accent text-white text-sm font-medium rounded-md hover:bg-accent-hover transition-colors">
              Run Diagnostic
            </button>
          </div>
          <Card className="p-5">
            <h3 className="text-xs font-semibold text-txt-tertiary uppercase tracking-wider mb-4">
              Last Diagnostic — Jan 15, 14:20
            </h3>
            <div className="space-y-3">
              {[
                {
                  metric: 'DNS Resolution',
                  value: '12ms',
                  status: 'good',
                },
                {
                  metric: 'Route Hops',
                  value: '8',
                  status: 'good',
                },
                {
                  metric: 'Packet Loss',
                  value: '0.0%',
                  status: 'good',
                },
                {
                  metric: 'TCP Window Scaling',
                  value: 'Enabled',
                  status: 'good',
                },
                {
                  metric: "Nagle's Algorithm",
                  value: 'Enabled',
                  status: 'warning',
                },
                {
                  metric: 'MTU Size',
                  value: '1500',
                  status: 'good',
                },
              ].map((item) => (
                <div
                  key={item.metric}
                  className="flex items-center justify-between py-2 border-b border-border-subtle last:border-0"
                >
                  <span className="text-sm text-txt-secondary">
                    {item.metric}
                  </span>
                  <div className="flex items-center gap-2">
                    <span className="text-sm font-mono text-txt-primary">
                      {item.value}
                    </span>
                    {item.status === 'good' ? (
                      <CheckCircleIcon className="w-4 h-4 text-success" />
                    ) : (
                      <AlertTriangleIcon className="w-4 h-4 text-warning" />
                    )}
                  </div>
                </div>
              ))}
            </div>
          </Card>
        </div>
      )}

      {activeTab === 'qos' && (
        <div className="space-y-5">
          <Card className="p-5">
            <div className="flex items-center gap-3 mb-3">
              <h3 className="text-[15px] font-semibold text-txt-primary">
                QoS Packet Scheduling
              </h3>
              <StatusBadge status="ACTIVE" />
            </div>
            <p className="text-sm text-txt-secondary mb-4">
              Quality of Service policies prioritize gaming traffic over
              background downloads.
            </p>
            <div className="space-y-3">
              {[
                {
                  app: 'Gaming Traffic',
                  bandwidth: 'Priority',
                  dscp: '46 (EF)',
                },
                {
                  app: 'Streaming',
                  bandwidth: 'Normal',
                  dscp: '0 (BE)',
                },
                {
                  app: 'Downloads',
                  bandwidth: 'Throttled',
                  dscp: '0 (BE)',
                },
              ].map((rule) => (
                <div
                  key={rule.app}
                  className="flex items-center justify-between py-2 border-b border-border-subtle last:border-0"
                >
                  <span className="text-sm text-txt-primary">{rule.app}</span>
                  <div className="flex items-center gap-4 text-xs text-txt-tertiary">
                    <span>{rule.bandwidth}</span>
                    <span className="font-mono">DSCP {rule.dscp}</span>
                  </div>
                </div>
              ))}
            </div>
          </Card>
          {/* Premium upsell */}
          <div className="flex items-center gap-3 p-3 rounded-lg bg-premium-muted border border-premium/20">
            <span className="text-premium text-sm">✦</span>
            <span className="text-sm text-premium">
              Unlock custom QoS rules with Pro
            </span>
            <button className="ml-auto text-xs text-premium hover:underline">
              Learn More →
            </button>
          </div>
        </div>
      )}

      {activeTab === 'adapters' && (
        <div className="space-y-3">
          {[
            {
              name: 'Intel I225-V',
              type: 'Ethernet',
              status: 'Connected',
              ip: '192.168.1.42',
              mac: 'A4:BB:6D:12:34:56',
              speed: '1 Gbps',
              rss: true,
              offload: true,
              jumbo: false,
            },
            {
              name: 'Intel Wi-Fi 6E AX211',
              type: 'Wi-Fi',
              status: 'Disconnected',
              ip: '—',
              mac: 'B8:CC:7E:23:45:67',
              speed: '—',
              rss: true,
              offload: true,
              jumbo: false,
            },
          ].map((adapter) => (
            <Card key={adapter.name} className="p-5">
              <div className="flex items-center justify-between mb-3">
                <div className="flex items-center gap-3">
                  <h3 className="text-sm font-semibold text-txt-primary">
                    {adapter.name}
                  </h3>
                  <span className="text-xs text-txt-tertiary">
                    {adapter.type}
                  </span>
                  <span
                    className={`text-xs font-medium ${adapter.status === 'Connected' ? 'text-success' : 'text-txt-disabled'}`}
                  >
                    {adapter.status}
                  </span>
                </div>
                {adapter.speed !== '—' && (
                  <span className="text-xs font-mono text-txt-tertiary">
                    {adapter.speed}
                  </span>
                )}
              </div>
              <div className="grid grid-cols-2 gap-2 text-xs mb-3">
                <div>
                  <span className="text-txt-tertiary">IP: </span>
                  <span className="font-mono text-txt-secondary">
                    {adapter.ip}
                  </span>
                </div>
                <div>
                  <span className="text-txt-tertiary">MAC: </span>
                  <span className="font-mono text-txt-secondary">
                    {adapter.mac}
                  </span>
                </div>
              </div>
              <div className="flex items-center gap-3">
                <span className="text-[11px] text-txt-tertiary">
                  Capabilities:
                </span>
                {[
                  {
                    label: 'RSS',
                    supported: adapter.rss,
                  },
                  {
                    label: 'TCP Offload',
                    supported: adapter.offload,
                  },
                  {
                    label: 'Jumbo Frames',
                    supported: adapter.jumbo,
                  },
                ].map((cap) => (
                  <span
                    key={cap.label}
                    className={`text-[10px] font-medium px-2 py-0.5 rounded ${cap.supported ? 'bg-success-muted text-success' : 'bg-surface-hover text-txt-disabled'}`}
                  >
                    {cap.label}: {cap.supported ? 'Yes' : 'No'}
                  </span>
                ))}
              </div>
            </Card>
          ))}
        </div>
      )}
    </div>
  )
}

```
```pages/Optimizations.tsx
import React, { useState, memo } from 'react'
import {
  SearchIcon,
  ChevronDownIcon,
  ChevronRightIcon,
  InfoIcon,
  LockIcon,
} from 'lucide-react'
import { Card } from '../components/Card'
import { StatusBadge, BadgeStatus } from '../components/StatusBadge'
interface Tweak {
  id: string
  name: string
  description: string
  category: string
  risk: 'Low' | 'Medium' | 'High'
  impact: 'Low' | 'Medium' | 'High'
  badges: BadgeStatus[]
  status: 'active' | 'available' | 'reverted' | 'failed'
  appliedAgo?: string
  premium?: boolean
}
const tweaks: Tweak[] = [
  {
    id: '1',
    name: 'High Performance Power Plan',
    description:
      'Switches Windows power plan to High Performance for maximum CPU availability',
    category: 'Power Management',
    risk: 'Low',
    impact: 'Medium',
    badges: ['VALIDATED', 'ACTIVE'],
    status: 'active',
    appliedAgo: '2h ago',
  },
  {
    id: '2',
    name: 'Disable USB Selective Suspend',
    description:
      'Prevents USB power saving that causes device disconnections during gaming',
    category: 'Power Management',
    risk: 'Low',
    impact: 'Low',
    badges: ['VALIDATED'],
    status: 'available',
  },
  {
    id: '3',
    name: 'Disable Windows Search Indexing',
    description:
      'Stops background disk indexing that competes for I/O during gaming sessions',
    category: 'Storage',
    risk: 'Low',
    impact: 'Medium',
    badges: ['VALIDATED'],
    status: 'available',
  },
  {
    id: '4',
    name: "Disable Nagle's Algorithm",
    description:
      'Reduces network micro-latency by disabling TCP packet coalescing',
    category: 'Network',
    risk: 'Low',
    impact: 'Medium',
    badges: ['VALIDATED', 'ACTIVE'],
    status: 'active',
    appliedAgo: '1h ago',
  },
  {
    id: '5',
    name: 'Enable RSS (Receive Side Scaling)',
    description:
      'Distributes network packet processing across multiple CPU cores',
    category: 'Network',
    risk: 'Low',
    impact: 'Low',
    badges: ['VALIDATED', 'HARDWARE_SPECIFIC'],
    status: 'available',
  },
  {
    id: '6',
    name: 'GPU Shader Cache Optimization',
    description:
      'Configures shader cache size and location for optimal GPU performance',
    category: 'GPU',
    risk: 'Low',
    impact: 'High',
    badges: ['VALIDATED', 'AI_APPLIED', 'ACTIVE'],
    status: 'active',
    appliedAgo: '30m ago',
  },
  {
    id: '7',
    name: 'Disable Prefetch and Superfetch',
    description:
      'Stops Windows memory prefetching that can cause stuttering with SSDs',
    category: 'Memory',
    risk: 'Medium',
    impact: 'Medium',
    badges: ['VALIDATED'],
    status: 'reverted',
  },
  {
    id: '8',
    name: 'Advanced CPU Parking Control',
    description:
      'Fine-tune CPU core parking behavior for consistent multi-threaded performance',
    category: 'Power Management',
    risk: 'High',
    impact: 'High',
    badges: ['EXPERT_ONLY', 'PREMIUM'],
    status: 'available',
    premium: true,
  },
  {
    id: '9',
    name: 'Network Throttling Index',
    description:
      'Adjusts Windows network throttling for gaming traffic priority',
    category: 'Network',
    risk: 'Low',
    impact: 'Medium',
    badges: ['VALIDATED', 'ACTIVE'],
    status: 'active',
    appliedAgo: '2h ago',
  },
  {
    id: '10',
    name: 'Disable HPET Timer',
    description: 'Switches to TSC timer for lower-latency system timing',
    category: 'Power Management',
    risk: 'High',
    impact: 'Low',
    badges: ['PLACEBO_RISK', 'LEGACY'],
    status: 'available',
  },
  {
    id: '11',
    name: 'SSD TRIM Optimization',
    description:
      'Ensures TRIM commands are properly scheduled for SSD longevity',
    category: 'Storage',
    risk: 'Low',
    impact: 'Low',
    badges: ['SAFE_DEFAULT', 'ACTIVE'],
    status: 'active',
    appliedAgo: '3d ago',
  },
  {
    id: '12',
    name: 'Game Mode Enhancement',
    description: 'Configures Windows Game Mode with optimal priority settings',
    category: 'Services',
    risk: 'Low',
    impact: 'Medium',
    badges: ['VALIDATED'],
    status: 'failed',
  },
]
type FilterTab = 'all' | 'active' | 'available' | 'reverted' | 'failed'
export function Optimizations() {
  const [activeTab, setActiveTab] = useState<FilterTab>('all')
  const [search, setSearch] = useState('')
  const [expandedCategories, setExpandedCategories] = useState<Set<string>>(
    new Set([
      'Power Management',
      'Network',
      'Storage',
      'GPU',
      'Memory',
      'Services',
    ]),
  )
  const [selectedTweak, setSelectedTweak] = useState<Tweak | null>(null)
  const tabs: {
    id: FilterTab
    label: string
  }[] = [
    {
      id: 'all',
      label: 'All',
    },
    {
      id: 'active',
      label: 'Active',
    },
    {
      id: 'available',
      label: 'Available',
    },
    {
      id: 'reverted',
      label: 'Reverted',
    },
    {
      id: 'failed',
      label: 'Failed',
    },
  ]
  const filtered = tweaks.filter((t) => {
    if (activeTab !== 'all' && t.status !== activeTab) return false
    if (
      search &&
      !t.name.toLowerCase().includes(search.toLowerCase()) &&
      !t.description.toLowerCase().includes(search.toLowerCase())
    )
      return false
    return true
  })
  const categories = [...new Set(filtered.map((t) => t.category))]
  const counts: Record<FilterTab, number> = {
    all: tweaks.length,
    active: tweaks.filter((t) => t.status === 'active').length,
    available: tweaks.filter((t) => t.status === 'available').length,
    reverted: tweaks.filter((t) => t.status === 'reverted').length,
    failed: tweaks.filter((t) => t.status === 'failed').length,
  }
  const toggleCategory = (cat: string) => {
    const next = new Set(expandedCategories)
    if (next.has(cat)) next.delete(cat)
    else next.add(cat)
    setExpandedCategories(next)
  }
  const riskColor = (risk: string) =>
    risk === 'Low'
      ? 'text-success'
      : risk === 'Medium'
        ? 'text-warning'
        : 'text-error'
  const riskDot = (risk: string) =>
    risk === 'Low'
      ? 'bg-success'
      : risk === 'Medium'
        ? 'bg-warning'
        : 'bg-error'
  return (
    <div className="flex gap-0 h-full">
      {/* Main Content */}
      <div
        className={`flex-1 min-w-0 space-y-5 ${selectedTweak ? 'pr-5' : ''}`}
      >
        {/* Header */}
        <div className="flex items-center justify-between">
          <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
            Optimizations
          </h1>
          <button className="px-4 py-1.5 bg-accent text-white text-sm font-medium rounded-md hover:bg-accent-hover transition-colors">
            Apply All Safe
          </button>
        </div>

        {/* Filter Bar */}
        <div className="flex items-center justify-between gap-4">
          <div className="flex items-center gap-0 border-b border-border-subtle">
            {tabs.map((tab) => (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id)}
                className={`px-3 py-2 text-sm transition-colors relative ${activeTab === tab.id ? 'text-txt-primary' : 'text-txt-tertiary hover:text-txt-secondary'}`}
              >
                {tab.label}
                <span className="ml-1.5 text-[10px] text-txt-disabled">
                  {counts[tab.id]}
                </span>
                {activeTab === tab.id && (
                  <span className="absolute bottom-0 left-0 right-0 h-0.5 bg-accent rounded-t" />
                )}
              </button>
            ))}
          </div>
          <div className="relative">
            <SearchIcon className="absolute left-2.5 top-1/2 -translate-y-1/2 w-3.5 h-3.5 text-txt-tertiary" />
            <input
              type="text"
              value={search}
              onChange={(e) => setSearch(e.target.value)}
              placeholder="Search optimizations..."
              className="pl-8 pr-3 py-1.5 text-sm bg-surface-sunken border border-border-default rounded-md text-txt-primary placeholder:text-txt-tertiary focus:outline-none focus:border-accent w-56"
            />
          </div>
        </div>

        {/* Optimization List */}
        <div className="space-y-4">
          {categories.map((category) => {
            const catTweaks = filtered.filter((t) => t.category === category)
            const isExpanded = expandedCategories.has(category)
            return (
              <div key={category}>
                <button
                  onClick={() => toggleCategory(category)}
                  className="flex items-center gap-2 mb-2 group"
                >
                  {isExpanded ? (
                    <ChevronDownIcon className="w-3.5 h-3.5 text-txt-tertiary" />
                  ) : (
                    <ChevronRightIcon className="w-3.5 h-3.5 text-txt-tertiary" />
                  )}
                  <span className="text-xs font-semibold text-txt-tertiary uppercase tracking-wider">
                    {category}
                  </span>
                  <span className="text-[10px] text-txt-disabled">
                    {catTweaks.length}
                  </span>
                </button>
                {isExpanded && (
                  <div className="space-y-2">
                    {catTweaks.map((tweak) => (
                      <Card
                        key={tweak.id}
                        className={`p-4 ${selectedTweak?.id === tweak.id ? 'border-accent/40' : ''} ${tweak.premium ? 'opacity-80' : ''}`}
                        statusColor={
                          tweak.status === 'failed'
                            ? '#f87171'
                            : tweak.status === 'active'
                              ? '#34d399'
                              : undefined
                        }
                      >
                        <div className="flex items-start gap-3">
                          <div className="flex-1 min-w-0">
                            <div className="flex items-center gap-2 mb-1">
                              {tweak.premium && (
                                <LockIcon className="w-3.5 h-3.5 text-premium flex-shrink-0" />
                              )}
                              <h4 className="text-sm font-semibold text-txt-primary truncate">
                                {tweak.name}
                              </h4>
                              <div className="flex items-center gap-1.5 flex-shrink-0">
                                {tweak.badges.slice(0, 3).map((b) => (
                                  <StatusBadge key={b} status={b} />
                                ))}
                              </div>
                            </div>
                            <p className="text-xs text-txt-secondary truncate mb-2">
                              {tweak.description}
                            </p>
                            <div className="flex items-center gap-3 text-[11px] text-txt-tertiary">
                              <span className="flex items-center gap-1">
                                <span
                                  className={`w-1.5 h-1.5 rounded-full ${riskDot(tweak.risk)}`}
                                />
                                <span className={riskColor(tweak.risk)}>
                                  Risk: {tweak.risk}
                                </span>
                              </span>
                              <span>Impact: {tweak.impact}</span>
                              {tweak.appliedAgo && (
                                <span>Applied {tweak.appliedAgo}</span>
                              )}
                            </div>
                          </div>
                          <div className="flex items-center gap-2 flex-shrink-0">
                            {tweak.status === 'available' && !tweak.premium && (
                              <button className="px-3 py-1 text-xs border border-border-default text-txt-primary rounded-md hover:bg-surface-hover transition-colors">
                                Apply
                              </button>
                            )}
                            {tweak.status === 'active' && (
                              <button className="px-3 py-1 text-xs text-txt-secondary hover:text-txt-primary transition-colors">
                                Revert
                              </button>
                            )}
                            {tweak.status === 'failed' && (
                              <button className="px-3 py-1 text-xs text-error hover:text-error/80 transition-colors">
                                Retry
                              </button>
                            )}
                            {tweak.premium && (
                              <button className="px-3 py-1 text-xs bg-premium-muted text-premium rounded-md hover:bg-premium/20 transition-colors">
                                Unlock Pro
                              </button>
                            )}
                            <button
                              onClick={() =>
                                setSelectedTweak(
                                  selectedTweak?.id === tweak.id ? null : tweak,
                                )
                              }
                              className="px-2 py-1 text-xs text-txt-tertiary hover:text-txt-secondary transition-colors"
                            >
                              Details →
                            </button>
                          </div>
                        </div>
                      </Card>
                    ))}
                  </div>
                )}
              </div>
            )
          })}
        </div>
      </div>

      {/* Detail Drawer */}
      {selectedTweak && (
        <div className="w-[380px] flex-shrink-0 bg-surface-elevated border-l border-border-subtle overflow-y-auto rounded-r-lg">
          <div className="p-5 border-b border-border-subtle flex items-center justify-between">
            <h3 className="text-sm font-semibold text-txt-primary truncate pr-2">
              {selectedTweak.name}
            </h3>
            <button
              onClick={() => setSelectedTweak(null)}
              className="text-txt-tertiary hover:text-txt-primary text-lg leading-none"
            >
              ×
            </button>
          </div>
          <div className="p-5 space-y-5">
            {/* Status */}
            <div>
              <div className="text-[11px] font-medium text-txt-tertiary uppercase tracking-wider mb-2">
                Status
              </div>
              <div className="flex flex-wrap gap-1.5">
                {selectedTweak.badges.map((b) => (
                  <StatusBadge key={b} status={b} />
                ))}
              </div>
              {selectedTweak.appliedAgo && (
                <div className="text-xs text-txt-tertiary mt-2">
                  Applied {selectedTweak.appliedAgo}
                </div>
              )}
            </div>

            {/* Description */}
            <div>
              <div className="text-[11px] font-medium text-txt-tertiary uppercase tracking-wider mb-2">
                Description
              </div>
              <p className="text-sm text-txt-secondary leading-relaxed">
                {selectedTweak.description}
              </p>
            </div>

            {/* Rationale */}
            <div>
              <div className="text-[11px] font-medium text-txt-tertiary uppercase tracking-wider mb-2">
                Rationale
              </div>
              <p className="text-sm text-txt-secondary leading-relaxed">
                This optimization targets a known Windows behavior that can
                negatively impact gaming performance. The change is
                well-documented and widely validated across similar hardware
                configurations.
              </p>
            </div>

            {/* Risk Assessment */}
            <div>
              <div className="text-[11px] font-medium text-txt-tertiary uppercase tracking-wider mb-2">
                Risk Assessment
              </div>
              <div className="space-y-2 text-sm">
                <div className="flex justify-between">
                  <span className="text-txt-tertiary">Risk</span>
                  <span
                    className={`font-medium ${riskColor(selectedTweak.risk)}`}
                  >
                    {selectedTweak.risk}
                  </span>
                </div>
                <div className="flex justify-between">
                  <span className="text-txt-tertiary">Impact</span>
                  <span className="text-txt-primary font-medium">
                    {selectedTweak.impact}
                  </span>
                </div>
                <div className="flex justify-between">
                  <span className="text-txt-tertiary">Reversible</span>
                  <span className="text-success font-medium">
                    Yes (instant)
                  </span>
                </div>
                <div className="flex justify-between">
                  <span className="text-txt-tertiary">Requires restart</span>
                  <span className="text-txt-primary font-medium">No</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-txt-tertiary">Requires admin</span>
                  <span className="text-warning font-medium">Yes</span>
                </div>
              </div>
            </div>

            {/* Before / After */}
            <div>
              <div className="text-[11px] font-medium text-txt-tertiary uppercase tracking-wider mb-2">
                Before / After
              </div>
              <div className="bg-surface-sunken rounded-md p-3 space-y-1.5">
                <div className="text-[11px] text-txt-tertiary font-mono">
                  HKLM\SYSTEM\CurrentControlSet\...
                </div>
                <div className="flex items-center gap-2 text-sm">
                  <span className="text-txt-tertiary">Before:</span>
                  <span className="font-mono text-error">1 (enabled)</span>
                </div>
                <div className="flex items-center gap-2 text-sm">
                  <span className="text-txt-tertiary">After:</span>
                  <span className="font-mono text-success">0 (disabled)</span>
                </div>
              </div>
            </div>

            {/* History */}
            <div>
              <div className="text-[11px] font-medium text-txt-tertiary uppercase tracking-wider mb-2">
                History
              </div>
              <div className="space-y-2">
                {[
                  {
                    date: 'Jan 15, 14:32',
                    action: 'Applied',
                    color: 'bg-accent',
                  },
                  {
                    date: 'Jan 14, 09:15',
                    action: 'Reverted',
                    color: 'bg-surface-active',
                  },
                  {
                    date: 'Jan 13, 22:00',
                    action: 'Applied',
                    color: 'bg-accent',
                  },
                ].map((h, i) => (
                  <div key={i} className="flex items-center gap-2.5 text-xs">
                    <span className={`w-1.5 h-1.5 rounded-full ${h.color}`} />
                    <span className="text-txt-tertiary font-mono">
                      {h.date}
                    </span>
                    <span className="text-txt-secondary">{h.action}</span>
                  </div>
                ))}
              </div>
            </div>

            {/* Actions */}
            <div className="pt-3 border-t border-border-subtle flex justify-end gap-2">
              {selectedTweak.status === 'active' && (
                <button className="px-4 py-1.5 text-sm border border-border-default text-txt-primary rounded-md hover:bg-surface-hover transition-colors">
                  Revert
                </button>
              )}
              <button
                onClick={() => setSelectedTweak(null)}
                className="px-4 py-1.5 text-sm text-txt-tertiary hover:text-txt-secondary transition-colors"
              >
                Close
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  )
}

```
```pages/Settings.tsx
import React, { useState } from 'react'
import { Card } from '../components/Card'
type Section =
  | 'general'
  | 'session'
  | 'performance'
  | 'safety'
  | 'expert'
  | 'data'
const sections: {
  id: Section
  label: string
}[] = [
  {
    id: 'general',
    label: 'General',
  },
  {
    id: 'session',
    label: 'Session',
  },
  {
    id: 'performance',
    label: 'Performance',
  },
  {
    id: 'safety',
    label: 'Safety',
  },
  {
    id: 'expert',
    label: 'Expert',
  },
  {
    id: 'data',
    label: 'Data',
  },
]
function Toggle({
  enabled,
  onChange,
}: {
  enabled: boolean
  onChange: () => void
}) {
  return (
    <button
      onClick={onChange}
      className={`w-9 h-5 rounded-full flex items-center px-0.5 transition-colors flex-shrink-0 ${enabled ? 'bg-accent justify-end' : 'bg-surface-active justify-start'}`}
    >
      <div className="w-4 h-4 rounded-full bg-white shadow-sm" />
    </button>
  )
}
function SettingRow({
  label,
  description,
  children,
}: {
  label: string
  description?: string
  children: React.ReactNode
}) {
  return (
    <div className="flex items-center justify-between py-3 border-b border-border-subtle last:border-0">
      <div className="flex-1 min-w-0 mr-4">
        <div className="text-sm text-txt-primary">{label}</div>
        {description && (
          <div className="text-xs text-txt-tertiary mt-0.5">{description}</div>
        )}
      </div>
      {children}
    </div>
  )
}
export function Settings() {
  const [activeSection, setActiveSection] = useState<Section>('general')
  const [toggles, setToggles] = useState<Record<string, boolean>>({
    startWithWindows: true,
    minimizeToTray: true,
    checkUpdates: true,
    optimizationAlerts: true,
    benchmarkCompletion: true,
    recoveryWarnings: true,
    sessionTracking: true,
    autoOptimize: false,
    sessionSummary: true,
    backgroundMonitoring: true,
    autoSnapshot: true,
    revertOnFailure: true,
    adminPrompt: true,
    dryRunDefault: false,
    expertMode: false,
    experimentalTweaks: false,
    verboseLogging: false,
  })
  const toggle = (key: string) =>
    setToggles((prev) => ({
      ...prev,
      [key]: !prev[key],
    }))
  return (
    <div className="space-y-5">
      <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
        Settings
      </h1>

      <div className="flex gap-5">
        {/* Section Nav */}
        <nav className="w-40 flex-shrink-0 space-y-0.5">
          {sections.map((section) => (
            <button
              key={section.id}
              onClick={() => setActiveSection(section.id)}
              className={`w-full text-left px-3 py-2 text-sm rounded-md transition-colors ${activeSection === section.id ? 'bg-nav-active text-txt-primary' : 'text-txt-secondary hover:text-txt-primary hover:bg-surface-hover'}`}
            >
              {section.label}
            </button>
          ))}
        </nav>

        {/* Content */}
        <div className="flex-1 min-w-0">
          {activeSection === 'general' && (
            <Card className="p-5">
              <h3 className="text-[15px] font-semibold text-txt-primary mb-1">
                General
              </h3>
              <p className="text-xs text-txt-tertiary mb-4">
                Application startup and notification preferences
              </p>
              <div>
                <SettingRow
                  label="Start with Windows"
                  description="Launch PulseBoost when Windows starts"
                >
                  <Toggle
                    enabled={toggles.startWithWindows}
                    onChange={() => toggle('startWithWindows')}
                  />
                </SettingRow>
                <SettingRow
                  label="Minimize to tray"
                  description="Keep running in system tray when closed"
                >
                  <Toggle
                    enabled={toggles.minimizeToTray}
                    onChange={() => toggle('minimizeToTray')}
                  />
                </SettingRow>
                <SettingRow
                  label="Check for updates"
                  description="Automatically check for new versions"
                >
                  <Toggle
                    enabled={toggles.checkUpdates}
                    onChange={() => toggle('checkUpdates')}
                  />
                </SettingRow>
                <SettingRow label="Update channel">
                  <select className="px-3 py-1 text-sm bg-surface-sunken border border-border-default rounded-md text-txt-primary focus:outline-none focus:border-accent">
                    <option>Stable</option>
                    <option>Beta</option>
                  </select>
                </SettingRow>
                <SettingRow label="Theme">
                  <select className="px-3 py-1 text-sm bg-surface-sunken border border-border-default rounded-md text-txt-primary focus:outline-none focus:border-accent">
                    <option>Dark</option>
                    <option disabled>Light (coming soon)</option>
                  </select>
                </SettingRow>
              </div>
              <div className="mt-5 pt-4 border-t border-border-subtle">
                <h4 className="text-xs font-semibold text-txt-tertiary uppercase tracking-wider mb-3">
                  Notifications
                </h4>
                <SettingRow label="Optimization alerts">
                  <Toggle
                    enabled={toggles.optimizationAlerts}
                    onChange={() => toggle('optimizationAlerts')}
                  />
                </SettingRow>
                <SettingRow label="Benchmark completion">
                  <Toggle
                    enabled={toggles.benchmarkCompletion}
                    onChange={() => toggle('benchmarkCompletion')}
                  />
                </SettingRow>
                <SettingRow label="Recovery warnings">
                  <Toggle
                    enabled={toggles.recoveryWarnings}
                    onChange={() => toggle('recoveryWarnings')}
                  />
                </SettingRow>
              </div>
            </Card>
          )}

          {activeSection === 'session' && (
            <Card className="p-5">
              <h3 className="text-[15px] font-semibold text-txt-primary mb-1">
                Session
              </h3>
              <p className="text-xs text-txt-tertiary mb-4">
                Session tracking and automation
              </p>
              <SettingRow
                label="Session duration tracking"
                description="Track how long optimization sessions run"
              >
                <Toggle
                  enabled={toggles.sessionTracking}
                  onChange={() => toggle('sessionTracking')}
                />
              </SettingRow>
              <SettingRow
                label="Auto-optimize on session start"
                description="Apply recommended tweaks when a session begins"
              >
                <Toggle
                  enabled={toggles.autoOptimize}
                  onChange={() => toggle('autoOptimize')}
                />
              </SettingRow>
              <SettingRow
                label="Session summary on close"
                description="Show optimization summary when ending a session"
              >
                <Toggle
                  enabled={toggles.sessionSummary}
                  onChange={() => toggle('sessionSummary')}
                />
              </SettingRow>
              <SettingRow
                label="Idle detection"
                description="Minutes of inactivity before session pauses"
              >
                <select className="px-3 py-1 text-sm bg-surface-sunken border border-border-default rounded-md text-txt-primary focus:outline-none focus:border-accent">
                  <option>15 minutes</option>
                  <option>30 minutes</option>
                  <option>1 hour</option>
                  <option>Never</option>
                </select>
              </SettingRow>
            </Card>
          )}

          {activeSection === 'performance' && (
            <Card className="p-5">
              <h3 className="text-[15px] font-semibold text-txt-primary mb-1">
                Performance
              </h3>
              <p className="text-xs text-txt-tertiary mb-4">
                Telemetry and monitoring configuration
              </p>
              <SettingRow label="Telemetry polling interval">
                <select className="px-3 py-1 text-sm bg-surface-sunken border border-border-default rounded-md text-txt-primary focus:outline-none focus:border-accent">
                  <option>1 second</option>
                  <option>2 seconds</option>
                  <option>5 seconds</option>
                </select>
              </SettingRow>
              <SettingRow label="Chart data retention">
                <select className="px-3 py-1 text-sm bg-surface-sunken border border-border-default rounded-md text-txt-primary focus:outline-none focus:border-accent">
                  <option>1 hour</option>
                  <option>6 hours</option>
                  <option>24 hours</option>
                </select>
              </SettingRow>
              <SettingRow
                label="Background monitoring"
                description="Continue monitoring when PulseBoost is minimized"
              >
                <Toggle
                  enabled={toggles.backgroundMonitoring}
                  onChange={() => toggle('backgroundMonitoring')}
                />
              </SettingRow>
            </Card>
          )}

          {activeSection === 'safety' && (
            <Card className="p-5">
              <h3 className="text-[15px] font-semibold text-txt-primary mb-1">
                Safety
              </h3>
              <p className="text-xs text-txt-tertiary mb-4">
                Recovery and protection settings
              </p>
              <SettingRow label="Auto-snapshot before batch operations">
                <Toggle
                  enabled={toggles.autoSnapshot}
                  onChange={() => toggle('autoSnapshot')}
                />
              </SettingRow>
              <SettingRow
                label="Revert on failure"
                description="Automatically revert if an optimization fails"
              >
                <Toggle
                  enabled={toggles.revertOnFailure}
                  onChange={() => toggle('revertOnFailure')}
                />
              </SettingRow>
              <SettingRow
                label="Admin elevation prompt"
                description="Ask before requesting administrator access"
              >
                <Toggle
                  enabled={toggles.adminPrompt}
                  onChange={() => toggle('adminPrompt')}
                />
              </SettingRow>
              <SettingRow
                label="Dry-run by default"
                description="Simulate changes without applying them"
              >
                <Toggle
                  enabled={toggles.dryRunDefault}
                  onChange={() => toggle('dryRunDefault')}
                />
              </SettingRow>
              <SettingRow label="Maximum simultaneous tweaks">
                <select className="px-3 py-1 text-sm bg-surface-sunken border border-border-default rounded-md text-txt-primary focus:outline-none focus:border-accent">
                  <option>20</option>
                  <option>50</option>
                  <option>Unlimited</option>
                </select>
              </SettingRow>
            </Card>
          )}

          {activeSection === 'expert' && (
            <Card className="p-5">
              <h3 className="text-[15px] font-semibold text-txt-primary mb-1">
                Expert
              </h3>
              <p className="text-xs text-txt-tertiary mb-4">
                Advanced features for power users
              </p>
              <SettingRow
                label="Expert mode"
                description="Show advanced optimizations with higher risk"
              >
                <Toggle
                  enabled={toggles.expertMode}
                  onChange={() => toggle('expertMode')}
                />
              </SettingRow>
              <SettingRow
                label="Show experimental tweaks"
                description="Include unvalidated optimizations"
              >
                <Toggle
                  enabled={toggles.experimentalTweaks}
                  onChange={() => toggle('experimentalTweaks')}
                />
              </SettingRow>
              <SettingRow
                label="Verbose logging"
                description="Detailed logs for troubleshooting"
              >
                <Toggle
                  enabled={toggles.verboseLogging}
                  onChange={() => toggle('verboseLogging')}
                />
              </SettingRow>
            </Card>
          )}

          {activeSection === 'data' && (
            <Card className="p-5">
              <h3 className="text-[15px] font-semibold text-txt-primary mb-1">
                Data
              </h3>
              <p className="text-xs text-txt-tertiary mb-4">
                Export, import, and manage your data
              </p>
              <div className="space-y-3">
                <div className="flex items-center justify-between py-2">
                  <div>
                    <div className="text-sm text-txt-primary">
                      Export all data
                    </div>
                    <div className="text-xs text-txt-tertiary">
                      Download all settings, profiles, and history as JSON
                    </div>
                  </div>
                  <button className="px-3 py-1.5 text-xs border border-border-default text-txt-secondary rounded-md hover:bg-surface-hover transition-colors">
                    Export
                  </button>
                </div>
                <div className="flex items-center justify-between py-2 border-t border-border-subtle">
                  <div>
                    <div className="text-sm text-txt-primary">
                      Import settings
                    </div>
                    <div className="text-xs text-txt-tertiary">
                      Restore from a previous export
                    </div>
                  </div>
                  <button className="px-3 py-1.5 text-xs border border-border-default text-txt-secondary rounded-md hover:bg-surface-hover transition-colors">
                    Import
                  </button>
                </div>
                <div className="flex items-center justify-between py-2 border-t border-border-subtle">
                  <div>
                    <div className="text-sm text-txt-primary">
                      Clear benchmark history
                    </div>
                    <div className="text-xs text-txt-tertiary">
                      Remove all benchmark results
                    </div>
                  </div>
                  <button className="px-3 py-1.5 text-xs border border-error/30 text-error rounded-md hover:bg-error-muted transition-colors">
                    Clear
                  </button>
                </div>
                <div className="flex items-center justify-between py-2 border-t border-border-subtle">
                  <div>
                    <div className="text-sm text-txt-primary">
                      Reset all settings
                    </div>
                    <div className="text-xs text-txt-tertiary">
                      Restore factory defaults — this cannot be undone
                    </div>
                  </div>
                  <button className="px-3 py-1.5 text-xs border border-error/30 text-error rounded-md hover:bg-error-muted transition-colors">
                    Reset
                  </button>
                </div>
              </div>
            </Card>
          )}
        </div>
      </div>
    </div>
  )
}

```
```pages/TrustCenter.tsx
import React, { useState } from 'react'
import {
  ShieldIcon,
  ShieldCheckIcon,
  CheckIcon,
  AlertTriangleIcon,
  InfoIcon,
  ToggleLeftIcon,
  ToggleRightIcon,
} from 'lucide-react'
import { Card } from '../components/Card'
export function TrustCenter() {
  const [expertMode, setExpertMode] = useState(false)
  return (
    <div className="space-y-5">
      <div>
        <h1 className="text-2xl font-semibold tracking-tight text-txt-primary">
          Trust Center
        </h1>
        <p className="text-sm text-txt-secondary mt-1">
          Your system's safety posture and recovery readiness
        </p>
      </div>

      {/* Recovery Readiness */}
      <Card elevated className="p-6">
        <div className="flex items-center gap-5">
          <div className="w-14 h-14 rounded-xl bg-success-muted flex items-center justify-center flex-shrink-0">
            <ShieldCheckIcon className="w-7 h-7 text-success" />
          </div>
          <div className="flex-1">
            <div className="flex items-center gap-2 mb-2">
              <h2 className="text-lg font-semibold text-txt-primary">
                System Recovery: Ready
              </h2>
              <CheckIcon className="w-5 h-5 text-success" />
            </div>
            <div className="grid grid-cols-4 gap-4 text-sm">
              <div>
                <span className="text-txt-tertiary">Snapshot age</span>
                <div className="font-medium text-txt-primary mt-0.5">
                  2h 14m
                </div>
              </div>
              <div>
                <span className="text-txt-tertiary">Snapshot size</span>
                <div className="font-medium text-txt-primary mt-0.5">
                  4.2 MB
                </div>
              </div>
              <div>
                <span className="text-txt-tertiary">Active tweaks</span>
                <div className="font-medium text-txt-primary mt-0.5">
                  14 (all reversible)
                </div>
              </div>
              <div>
                <span className="text-txt-tertiary">Non-reversible</span>
                <div className="font-medium text-success mt-0.5">0</div>
              </div>
            </div>
          </div>
        </div>
        <div className="flex items-center gap-3 mt-5 pt-5 border-t border-border-subtle">
          <button className="px-4 py-1.5 text-sm border border-border-default text-txt-primary rounded-md hover:bg-surface-hover transition-colors">
            Create Snapshot
          </button>
          <button className="px-4 py-1.5 text-sm border border-error/30 text-error rounded-md hover:bg-error-muted transition-colors">
            Revert All Changes
          </button>
        </div>
      </Card>

      <div className="grid grid-cols-2 gap-5">
        {/* Safeguards */}
        <Card className="p-5">
          <h3 className="text-[15px] font-semibold text-txt-primary mb-4">
            Safeguards
          </h3>
          <div className="space-y-3">
            {[
              {
                label: 'Auto-snapshot before batch operations',
                enabled: true,
              },
              {
                label: 'Revert on failure',
                enabled: true,
              },
              {
                label: 'Admin elevation prompt',
                enabled: true,
              },
              {
                label: 'Expert mode gating',
                enabled: true,
              },
              {
                label: 'Dry-run by default',
                enabled: false,
              },
            ].map((safeguard) => (
              <div
                key={safeguard.label}
                className="flex items-center justify-between"
              >
                <span className="text-sm text-txt-secondary">
                  {safeguard.label}
                </span>
                <div
                  className={`w-9 h-5 rounded-full flex items-center px-0.5 cursor-pointer transition-colors ${safeguard.enabled ? 'bg-accent justify-end' : 'bg-surface-active justify-start'}`}
                >
                  <div className="w-4 h-4 rounded-full bg-white shadow-sm" />
                </div>
              </div>
            ))}
          </div>
        </Card>

        {/* Protected Processes */}
        <Card className="p-5">
          <h3 className="text-[15px] font-semibold text-txt-primary mb-4">
            Protected Processes
          </h3>
          <div className="space-y-2.5">
            {[
              'Windows Update',
              'Windows Defender',
              'Windows Firewall',
              'BitLocker',
              'Windows Security',
            ].map((process) => (
              <div
                key={process}
                className="flex items-center justify-between py-1"
              >
                <span className="text-sm text-txt-secondary">{process}</span>
                <span className="text-[10px] font-semibold uppercase tracking-wider text-success bg-success-muted px-2 py-0.5 rounded">
                  Protected
                </span>
              </div>
            ))}
          </div>
          <p className="text-xs text-txt-tertiary mt-4 pt-3 border-t border-border-subtle">
            PulseBoost will never modify these services.
          </p>
        </Card>
      </div>

      {/* Limitations */}
      <Card className="p-5">
        <h3 className="text-[15px] font-semibold text-txt-primary mb-3">
          Limitations & Transparency
        </h3>
        <p className="text-sm text-txt-secondary mb-3">
          PulseBoost modifies Windows registry, services, and power settings. It
          does NOT modify:
        </p>
        <div className="grid grid-cols-2 gap-2">
          {[
            'BIOS/UEFI settings',
            'GPU firmware or vBIOS',
            'Hardware voltage or clock speeds',
            'Third-party application settings',
            'Windows core system files',
            'Boot configuration data',
          ].map((item) => (
            <div
              key={item}
              className="flex items-center gap-2 text-sm text-txt-tertiary"
            >
              <span className="w-1 h-1 rounded-full bg-txt-tertiary flex-shrink-0" />
              {item}
            </div>
          ))}
        </div>
        <p className="text-xs text-txt-tertiary mt-3 pt-3 border-t border-border-subtle">
          All changes are logged in the Audit Log.
        </p>
      </Card>

      {/* Expert Mode */}
      <Card className="p-5">
        <div className="flex items-center justify-between mb-3">
          <h3 className="text-[15px] font-semibold text-txt-primary">
            Expert Mode
          </h3>
          <button
            onClick={() => setExpertMode(!expertMode)}
            className={`w-9 h-5 rounded-full flex items-center px-0.5 transition-colors ${expertMode ? 'bg-warning justify-end' : 'bg-surface-active justify-start'}`}
          >
            <div className="w-4 h-4 rounded-full bg-white shadow-sm" />
          </button>
        </div>
        <p className="text-sm text-txt-secondary mb-3">
          Expert mode unlocks advanced optimizations that carry higher risk.
          These tweaks are hidden by default.
        </p>
        {!expertMode ? (
          <div className="space-y-2 text-sm text-txt-tertiary">
            <div className="flex items-center gap-2">
              <span className="w-1 h-1 rounded-full bg-txt-tertiary" />
              EXPERT_ONLY tweaks become visible
            </div>
            <div className="flex items-center gap-2">
              <span className="w-1 h-1 rounded-full bg-txt-tertiary" />
              Additional confirmation steps for risky actions
            </div>
            <div className="flex items-center gap-2">
              <span className="w-1 h-1 rounded-full bg-txt-tertiary" />
              Dry-run mode available for testing
            </div>
          </div>
        ) : (
          <div className="flex items-start gap-2 p-3 rounded-md bg-warning-muted">
            <AlertTriangleIcon className="w-4 h-4 text-warning mt-0.5 flex-shrink-0" />
            <p className="text-sm text-warning">
              Expert mode is active. Advanced optimizations are now visible and
              may affect system stability.
            </p>
          </div>
        )}
      </Card>
    </div>
  )
}

```
```PULSEBOOST_DESIGN_SPEC.md

# PulseBoost — Complete UI/UX Design Specification
### Premium Windows Optimization & Performance Product
---

## Table of Contents

1. [Design Philosophy](#1-design-philosophy)
2. [Design System](#2-design-system)
3. [Global Shell & Navigation](#3-global-shell--navigation)
4. [Status & Badge System](#4-status--badge-system)
5. [Screen Specifications](#5-screen-specifications)
   - 5.1 Dashboard
   - 5.2 Optimizations
   - 5.3 Network
   - 5.4 GPU
   - 5.5 Audit Log
   - 5.6 Benchmark
   - 5.7 Game Profiles
   - 5.8 Trust Center
   - 5.9 Settings
   - 5.10 Account / Plan / Licensing
6. [Flow Specifications](#6-flow-specifications)
7. [Monetization Patterns](#7-monetization-patterns)
8. [Microcopy Guidelines](#8-microcopy-guidelines)
9. [Interaction & Motion Principles](#9-interaction--motion-principles)
10. [Accessibility & Keyboard Navigation](#10-accessibility--keyboard-navigation)

---

## 1. Design Philosophy

### Core Principles

| Principle | Meaning |
|---|---|
| **Trust First** | Every surface communicates safety, transparency, and reversibility before action |
| **Proof Before Hype** | Data, benchmarks, and audit trails — never unsubstantiated claims |
| **Calm Density** | Show less, reveal more on demand — progressive disclosure everywhere |
| **Desktop Native** | Feels like a native Windows app, not a browser page or SaaS dashboard |
| **Premium Restraint** | Every pixel earns its place — no decoration for decoration's sake |
| **Monetization with Dignity** | Plan-aware states feel natural, never scammy or desperate |

### Anti-Patterns (Explicitly Forbidden)

- ❌ SaaS dashboard aesthetic (card grids with KPI numbers)
- ❌ Browser/web page feel (scrolling content pages, breadcrumbs)
- ❌ Admin panel patterns (data tables as primary content)
- ❌ Debug console density (raw metrics, log dumps)
- ❌ Cyberpunk/gamer neon aesthetic
- ❌ Noisy gradients, glows, or particle effects
- ❌ Giant metrics walls with 20+ numbers visible
- ❌ Student project UI (inconsistent spacing, mixed patterns)

### Design DNA

PulseBoost should feel like the intersection of:
- **Precision instrument** (like a high-end audio interface)
- **Professional tool** (like Figma or Ableton — dense but calm)
- **Premium utility** (like 1Password or CleanMyMac — trustworthy and polished)

---

## 2. Design System

### 2.1 Color Palette

#### Backgrounds & Surfaces

| Token | Hex | Usage |
|---|---|---|
| `bg-app` | `#0f1117` | Application background, window chrome |
| `bg-surface` | `#1a1d27` | Primary content surfaces, cards, panels |
| `bg-surface-elevated` | `#1f2231` | Elevated cards, modals, dropdowns, drawers |
| `bg-surface-hover` | `#252838` | Hover state for interactive surfaces |
| `bg-surface-active` | `#2a2e40` | Active/pressed state for surfaces |
| `bg-surface-sunken` | `#13151c` | Inset areas, code blocks, input fields |
| `bg-nav` | `#14161f` | Navigation sidebar background |
| `bg-nav-active` | `#1c1f2d` | Active nav item background |

#### Borders

| Token | Hex | Usage |
|---|---|---|
| `border-default` | `#2a2d3a` | Standard borders, dividers |
| `border-subtle` | `#1f2231` | Subtle separation lines |
| `border-strong` | `#3a3d4a` | Emphasized borders (focus rings, active states) |

#### Text

| Token | Hex | Usage |
|---|---|---|
| `text-primary` | `#e8eaf0` | Primary text, headings |
| `text-secondary` | `#9ca0b0` | Secondary text, descriptions, labels |
| `text-tertiary` | `#6b7084` | Tertiary text, timestamps, metadata |
| `text-disabled` | `#4a4e60` | Disabled text |
| `text-inverse` | `#0f1117` | Text on light/accent backgrounds |

#### Accent & Brand

| Token | Hex | Usage |
|---|---|---|
| `accent-primary` | `#6366f1` | Primary accent — buttons, active states, links |
| `accent-primary-hover` | `#7577f5` | Hover state for primary accent |
| `accent-primary-muted` | `rgba(99,102,241,0.12)` | Muted accent backgrounds (badges, highlights) |

#### Semantic Colors (Restrained)

| Token | Hex | Usage |
|---|---|---|
| `status-success` | `#34d399` | Validated, healthy, applied successfully |
| `status-success-muted` | `rgba(52,211,153,0.10)` | Success badge backgrounds |
| `status-warning` | `#fbbf24` | Caution, placebo risk, requires attention |
| `status-warning-muted` | `rgba(251,191,36,0.10)` | Warning badge backgrounds |
| `status-error` | `#f87171` | Failed, error, critical |
| `status-error-muted` | `rgba(248,113,113,0.10)` | Error badge backgrounds |
| `status-info` | `#60a5fa` | Informational, AI-applied, temporary |
| `status-info-muted` | `rgba(96,165,250,0.10)` | Info badge backgrounds |
| `status-neutral` | `#6b7084` | Inactive, legacy, reverted |
| `status-neutral-muted` | `rgba(107,112,132,0.10)` | Neutral badge backgrounds |
| `status-premium` | `#a78bfa` | Premium/locked features |
| `status-premium-muted` | `rgba(167,139,250,0.10)` | Premium badge backgrounds |

### 2.2 Typography

**Font Stack:** `Inter` (primary), `JetBrains Mono` (monospace/technical values)

| Token | Size | Weight | Line Height | Letter Spacing | Usage |
|---|---|---|---|---|---|
| `display` | 24px | 600 | 32px | -0.02em | Page titles (one per screen) |
| `heading-lg` | 18px | 600 | 26px | -0.01em | Section headings |
| `heading-md` | 15px | 600 | 22px | -0.005em | Card titles, group labels |
| `heading-sm` | 13px | 600 | 18px | 0 | Sub-section titles |
| `body` | 13px | 400 | 20px | 0 | Primary body text |
| `body-sm` | 12px | 400 | 18px | 0 | Secondary descriptions |
| `caption` | 11px | 500 | 16px | 0.02em | Labels, timestamps, metadata |
| `mono` | 12px | 400 | 18px | 0 | Technical values, paths, IDs |
| `mono-sm` | 11px | 400 | 16px | 0 | Inline code, small technical |
| `badge` | 10px | 600 | 14px | 0.04em | Badge text (uppercase) |

**Rules:**
- Maximum 3 type sizes visible in any single view
- Headings always have at least 24px space above
- Body text never exceeds 65ch line length
- Monospace only for genuinely technical values (registry paths, hex values, IDs)

### 2.3 Spacing & Density

**Base unit:** 4px

| Token | Value | Usage |
|---|---|---|
| `space-1` | 4px | Inline icon gaps, badge padding |
| `space-2` | 8px | Tight element spacing |
| `space-3` | 12px | Standard element spacing |
| `space-4` | 16px | Card internal padding, section gaps |
| `space-5` | 20px | Between related groups |
| `space-6` | 24px | Between sections |
| `space-8` | 32px | Major section separation |
| `space-10` | 40px | Page-level vertical rhythm |
| `space-12` | 48px | Maximum separation |

**Density Rules:**
- Cards: 16px internal padding, 12px between internal elements
- Lists: 8px between items, 12px row padding vertical
- Navigation: 6px between items, 36px item height
- Page content area: 24px padding from edges
- Sidebar width: 220px (collapsed: 56px)

### 2.4 Elevation & Surfaces

| Level | Shadow | Usage |
|---|---|---|
| `elevation-0` | none | Flat surfaces, backgrounds |
| `elevation-1` | `0 1px 3px rgba(0,0,0,0.3), 0 1px 2px rgba(0,0,0,0.2)` | Cards, panels |
| `elevation-2` | `0 4px 12px rgba(0,0,0,0.35), 0 2px 4px rgba(0,0,0,0.2)` | Dropdowns, popovers |
| `elevation-3` | `0 8px 24px rgba(0,0,0,0.4), 0 4px 8px rgba(0,0,0,0.25)` | Modals, drawers |
| `elevation-4` | `0 16px 48px rgba(0,0,0,0.5)` | Overlay dialogs, command palette |

**Rules:**
- Maximum 2 elevation levels visible simultaneously
- Cards on `bg-surface` use `elevation-1`
- Modals/drawers use `elevation-3` with a `rgba(0,0,0,0.6)` backdrop
- No box-shadow on navigation — use border-right instead

### 2.5 Border Radius

| Token | Value | Usage |
|---|---|---|
| `radius-sm` | 4px | Badges, small tags |
| `radius-md` | 6px | Buttons, inputs, small cards |
| `radius-lg` | 8px | Cards, panels |
| `radius-xl` | 12px | Modals, drawers |
| `radius-full` | 9999px | Avatars, circular indicators |

### 2.6 Component Library

#### Buttons

| Variant | Background | Text | Border | Usage |
|---|---|---|---|---|
| **Primary** | `accent-primary` | `text-inverse` | none | Primary CTA (one per view) |
| **Secondary** | transparent | `text-primary` | `border-default` | Secondary actions |
| **Ghost** | transparent | `text-secondary` | none | Tertiary actions, icon buttons |
| **Danger** | transparent | `status-error` | `status-error` at 30% | Destructive actions |
| **Danger-Filled** | `status-error` | white | none | Confirmed destructive (inside confirmation dialogs only) |
| **Premium** | `status-premium-muted` | `status-premium` | none | Upgrade/unlock actions |

**Button Sizes:**
- `sm`: 28px height, 12px horizontal padding, `body-sm` text
- `md`: 32px height, 16px horizontal padding, `body` text
- `lg`: 36px height, 20px horizontal padding, `body` text

**Rules:**
- Maximum ONE primary button per visible view
- Danger buttons always require a confirmation step
- Icon-only buttons: 32×32px with `ghost` variant
- Loading state: spinner replaces icon, text dims to `text-tertiary`

#### Cards

| Variant | Usage |
|---|---|
| **Standard** | Default content container — `bg-surface`, `border-default`, `radius-lg` |
| **Elevated** | Highlighted/featured content — `bg-surface-elevated`, `elevation-1` |
| **Interactive** | Clickable cards — adds `bg-surface-hover` on hover, cursor pointer |
| **Status** | Cards with a left-edge 3px color bar indicating status |
| **Premium-Locked** | Standard card with 60% opacity content, lock icon overlay, upgrade CTA |

#### Inputs

- Height: 32px
- Background: `bg-surface-sunken`
- Border: `border-default`, focus: `accent-primary`
- Text: `text-primary`, placeholder: `text-tertiary`
- Label above (caption style), helper text below (body-sm, text-secondary)
- Error state: border `status-error`, helper text `status-error`

#### Toggles & Switches

- Width: 36px, Height: 20px
- Off: `bg-surface-active` track, `text-tertiary` dot
- On: `accent-primary` track, white dot
- Disabled: 40% opacity

#### Tabs

- Underline style (not pill/button tabs)
- Inactive: `text-secondary`, no underline
- Active: `text-primary`, 2px `accent-primary` underline
- Hover: `text-primary`, no underline
- Tab bar has a 1px `border-subtle` bottom border
- 32px tab height, 16px horizontal padding between tabs

#### Tables

- Header: `caption` style text, `text-tertiary`, uppercase, `bg-surface-sunken`
- Rows: 40px height, `border-subtle` bottom border
- Hover: `bg-surface-hover`
- Selected: `accent-primary-muted` background
- No zebra striping
- Sortable columns: chevron icon beside header text

#### Charts

- Background: transparent (sits on card surface)
- Grid lines: `border-subtle`, dashed, 1px
- Axis labels: `caption` style, `text-tertiary`
- Data lines: 2px stroke, `accent-primary` for primary metric
- Area fills: gradient from accent at 15% opacity to 0%
- Tooltip: `bg-surface-elevated`, `elevation-2`, `radius-md`
- No chart legends inline — use card header labels instead
- Maximum 3 data series per chart

#### Drawers (Detail Panels)

- Slides in from right edge
- Width: 420px (standard), 560px (wide/comparison)
- Background: `bg-surface-elevated`
- Header: title + close button, `border-subtle` bottom
- Content: scrollable, 20px padding
- Backdrop: `rgba(0,0,0,0.4)` (subtle, not heavy)

#### Modals (Confirmation Dialogs)

- Centered, max-width 440px
- Background: `bg-surface-elevated`, `elevation-4`
- Backdrop: `rgba(0,0,0,0.6)`
- Structure: Icon (optional) → Title → Description → Actions
- Actions: right-aligned, secondary left, primary/danger right
- Risky actions: require typing confirmation text or checkbox acknowledgment

#### Toasts / Notifications

- Position: bottom-right, 16px from edges
- Width: 360px
- Background: `bg-surface-elevated`, `elevation-2`
- Left edge: 3px color bar matching severity
- Auto-dismiss: 5s (info), 8s (warning), persistent (error)
- Stack: max 3 visible, newest on top
- Structure: icon + title + description (optional) + dismiss button

#### Empty States

- Centered in content area
- Muted icon (48px, `text-tertiary`)
- Heading (`heading-md`, `text-secondary`)
- Description (`body-sm`, `text-tertiary`)
- Optional CTA button
- No illustrations — keep it clean and technical

#### Skeleton Loading

- Pulse animation on `bg-surface-hover` rectangles
- Match the exact layout of the content being loaded
- 800ms pulse cycle, ease-in-out
- Never show spinners for page loads — always skeletons

---

## 3. Global Shell & Navigation

### 3.1 Window Chrome

PulseBoost uses a **custom title bar** integrated with the app shell:

```
┌─────────────────────────────────────────────────────────────────────┐
│ [PulseBoost Logo]  PulseBoost              ─  □  ✕                 │
│─────────────────────────────────────────────────────────────────────│
│ │                    │                                              │
│ │   NAVIGATION       │              CONTENT AREA                   │
│ │   SIDEBAR          │                                              │
│ │                    │                                              │
│ │                    │                                              │
│ │                    │                                              │
│ │                    │                                              │
│ │                    │                                              │
│ │                    │                                              │
│ │                    │                                              │
│ │   ─────────────    │                                              │
│ │   [Account]        │                                              │
│ │   [Settings]       │                                              │
└─────────────────────────────────────────────────────────────────────┘
```

**Title Bar (32px height):**
- Left: PulseBoost logomark (16px) + "PulseBoost" wordmark (`heading-sm`, `text-secondary`)
- Right: minimize / maximize / close (custom styled, not native)
- Background: `bg-app`
- Draggable region for window movement
- No menu bar — all actions are contextual

### 3.2 Navigation Sidebar

**Width:** 220px (expanded), 56px (collapsed — icon only)
**Background:** `bg-nav`
**Right border:** 1px `border-subtle`

**Structure:**

```
[Logo area — part of title bar]
─────────────────────
[Search / Command]     ← Cmd+K shortcut hint

[PRIMARY NAV]
  Dashboard            ← Activity icon
  Optimizations        ← Sliders icon
  Network              ← Globe icon
  GPU                  ← Cpu icon
  Audit Log            ← FileText icon
  Benchmark            ← BarChart3 icon
  Game Profiles        ← Gamepad2 icon
  Trust Center         ← Shield icon

─────────────────────  ← Subtle divider

[BOTTOM NAV]
  Settings             ← Settings icon
  ─────────────────
  [Account pill]       ← Avatar + name + plan badge
```

**Nav Item Anatomy (36px height):**
- 12px left padding
- 16px icon (`text-tertiary`, active: `accent-primary`)
- 8px gap
- Label (`body-sm`, `text-secondary`, active: `text-primary`)
- Optional right badge (notification count or "PRO" badge)
- Active state: `bg-nav-active` background, left 2px `accent-primary` bar
- Hover: `bg-surface-hover`

**Account Pill (bottom of sidebar):**
- 40px height
- Avatar circle (28px, initials or image)
- Name (`body-sm`, `text-primary`, truncated)
- Plan badge: "FREE" / "PRO" / "ENT" in `badge` style
- Click → navigates to Account page
- Signed-out state: "Sign In" link with user-plus icon

**Collapsed State:**
- Only icons visible, centered
- Tooltip on hover showing label
- Account pill becomes just avatar circle
- Toggle via chevron button at sidebar bottom or keyboard shortcut

### 3.3 Command Palette (Cmd+K)

- Overlay modal, centered, 520px wide
- Search input at top (40px height, large text)
- Results grouped: Pages, Actions, Recent
- Each result: icon + label + shortcut hint (right-aligned)
- Keyboard navigable (arrow keys + enter)
- Fuzzy search across all pages, actions, and settings

### 3.4 Content Area

- Background: `bg-app`
- Padding: 24px all sides
- Max content width: 1200px (centered on very wide screens)
- Page header: `display` title + optional subtitle (`body-sm`, `text-secondary`)
- Page header height: 56px (title + optional description)
- Content below header: scrollable independently

---

## 4. Status & Badge System

### 4.1 Badge Design

All badges follow a consistent anatomy:
- Height: 20px
- Padding: 4px 8px
- Border-radius: `radius-sm` (4px)
- Text: `badge` style (10px, 600 weight, uppercase, 0.04em tracking)
- Background: muted color variant
- Text color: full color variant
- Optional: dot indicator (6px circle) before text for active states

### 4.2 Badge Definitions

| Status | Background | Text Color | Dot | Meaning |
|---|---|---|---|---|
| `VALIDATED` | `status-success-muted` | `status-success` | ● green | Tweak verified by benchmark data |
| `LEGACY` | `status-neutral-muted` | `status-neutral` | — | Older tweak, may not apply to current hardware |
| `HARDWARE_SPECIFIC` | `status-info-muted` | `status-info` | — | Only applies to specific hardware configurations |
| `PLACEBO_RISK` | `status-warning-muted` | `status-warning` | — | May not produce measurable improvement |
| `ACTIVE` | `status-success-muted` | `status-success` | ● green | Currently applied and running |
| `REVERTED` | `status-neutral-muted` | `status-neutral` | — | Was applied, now rolled back |
| `FAILED` | `status-error-muted` | `status-error` | ● red | Application failed |
| `AI_APPLIED` | `accent-primary-muted` | `accent-primary` | — | Applied by adaptive optimization engine |
| `TEMPORARY` | `status-info-muted` | `status-info` | — | Will auto-revert after session/time |
| `REQUIRES_ADMIN` | `status-warning-muted` | `status-warning` | — | Needs elevated privileges |
| `UNSUPPORTED` | `status-neutral-muted` | `status-neutral` | — | Not available on this hardware/OS |
| `EXPERT_ONLY` | `status-warning-muted` | `status-warning` | — | Hidden unless expert mode enabled |
| `DRY_RUN` | `status-info-muted` | `status-info` | — | Simulated, no changes made |
| `LIVE` | `status-success-muted` | `status-success` | ● green (pulsing) | Real-time active session |
| `SAFE_DEFAULT` | `status-success-muted` | `status-success` | — | Factory/recommended setting |
| `RECOVERY_MODE` | `status-error-muted` | `status-error` | ● red (pulsing) | System in recovery state |
| `LOCKED` | `status-neutral-muted` | `status-neutral` | 🔒 icon | Requires higher plan tier |
| `PREMIUM` | `status-premium-muted` | `status-premium` | ✦ icon | Premium feature |

### 4.3 Badge Grouping Rules

- Maximum 3 badges per item in list views
- In detail views/drawers, show all applicable badges
- Order: severity first (FAILED > RECOVERY > ACTIVE > others), then alphabetical
- Badges wrap to second line if needed (never truncate)

### 4.4 Status Indicators (Non-Badge)

For system-level status (not per-item):

| Indicator | Visual | Usage |
|---|---|---|
| **Health dot** | 8px circle, color-coded | System health in dashboard header |
| **Progress ring** | 24px circular progress, `accent-primary` | Ongoing operations |
| **Pulse dot** | 8px circle with CSS pulse animation | Live/active sessions |
| **Severity bar** | 3px left border on cards | Card-level status indication |

---

## 5. Screen Specifications

---

### 5.1 Dashboard

**Philosophy:** The dashboard is a flagship landing page — a calm, confident summary of machine state. It answers three questions: "Is my system healthy?", "What's happening right now?", and "What should I do next?"

**Layout:**

```
┌──────────────────────────────────────────────────────────────────┐
│  Dashboard                                          [LIVE ●]    │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  MACHINE STATE HERO                                        │  │
│  │                                                            │  │
│  │  [Health Ring]     System Health: Excellent                 │  │
│  │   92/100           Session: Active · 2h 14m                │  │
│  │                    Last optimization: 12 min ago            │  │
│  │                    Recovery: Ready                          │  │
│  │                                                            │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌─────────────────────┐  ┌─────────────────────┐               │
│  │  TOP TELEMETRY       │  │  RECOMMENDATIONS     │              │
│  │                      │  │                      │              │
│  │  CPU: 34% · 62°C    │  │  3 optimizations     │              │
│  │  GPU: 12% · 48°C    │  │  available            │              │
│  │  RAM: 67%           │  │                      │              │
│  │  Disk: 45%          │  │  [View All →]        │              │
│  │                      │  │                      │              │
│  │  [Expand ↓]         │  │  ● Disable indexing  │              │
│  └─────────────────────┘  │  ● Adjust power plan │              │
│                            │  ● Update GPU driver │              │
│  ┌─────────────────────┐  │                      │              │
│  │  TRUST POSTURE       │  └─────────────────────┘              │
│  │                      │                                        │
│  │  Rollback: Ready ✓  │                                        │
│  │  Snapshot: 2h ago   │                                        │
│  │  Active tweaks: 14  │                                        │
│  │  Reverted: 2        │                                        │
│  │                      │                                        │
│  │  [Trust Center →]   │                                        │
│  └─────────────────────┘                                        │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

**Machine State Hero (top section, full width):**
- Single large card, `bg-surface`, `elevation-1`
- Left: Health score ring (64px diameter, `accent-primary` arc, score number centered)
- Right of ring: 
  - "System Health: [Excellent/Good/Fair/Poor]" — `heading-lg`
  - Session status line — `body-sm`, `text-secondary`
  - Last optimization timestamp — `body-sm`, `text-tertiary`
  - Recovery readiness — `body-sm` with green/amber/red dot
- This is the ONLY large visual element on the page
- No charts, no graphs, no sparklines in the hero

**Below Hero — Two-Column Layout:**

**Left Column (60% width):**

**Top Telemetry Card:**
- `heading-md` title: "System"
- 4 key metrics only: CPU, GPU, RAM, Disk
- Each metric: label (`body-sm`, `text-secondary`) + value (`heading-sm`, `text-primary`) + small bar (40px wide, 4px tall, color-coded)
- Temperature shown inline where applicable (`mono-sm`, `text-tertiary`)
- "Expand" link at bottom → reveals secondary metrics (network throughput, process count, uptime) via collapse animation
- NO real-time charts on dashboard — charts live in dedicated pages

**Trust Posture Card:**
- `heading-md` title: "Trust & Recovery"
- 4 key-value pairs in a compact list:
  - Rollback readiness (Ready ✓ / Not Ready ⚠)
  - Last snapshot age
  - Active tweaks count
  - Reverted tweaks count
- "Trust Center →" link at bottom

**Right Column (40% width):**

**Recommendations Card:**
- `heading-md` title: "Recommendations"
- Count badge: "3 available"
- List of top 3 recommendations, each:
  - Dot indicator (accent color)
  - Short description (`body`, `text-primary`)
  - Risk badge if applicable
  - No action buttons here — just awareness
- "View All →" link → navigates to Optimizations page
- If no recommendations: empty state with "System is well-optimized" message

**What the Dashboard Does NOT Show:**
- No detailed charts or graphs
- No optimization history
- No audit log entries
- No benchmark results
- No network diagnostics
- No GPU details
- No game profile information
- No settings
- All of these live in their dedicated pages

**Dashboard States:**
- **First launch (no data):** Hero shows "Analyzing your system..." with skeleton loading, recommendations show "Scanning for optimizations..."
- **Session active:** LIVE badge pulses in page header
- **Recovery mode:** Hero ring turns red, "Recovery Mode Active" warning banner above hero
- **Free tier:** Recommendations may show PREMIUM badges on locked items

---

### 5.2 Optimizations

**Philosophy:** The command center for all system tweaks. Premium tweak management with scientific rigor — every optimization shows its rationale, risk level, and proof.

**Layout:**

```
┌──────────────────────────────────────────────────────────────────┐
│  Optimizations                              [Apply All Safe ▾]  │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │ [All] [Active] [Available] [Reverted] [Failed]    🔍 Search ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌─── Category: Power Management ──────────────────────────────┐│
│  │                                                              ││
│  │  ┌────────────────────────────────────────────────────────┐  ││
│  │  │ ○ High Performance Power Plan          VALIDATED ACTIVE│  ││
│  │  │   Switches Windows power plan to High Performance      │  ││
│  │  │   Risk: Low · Impact: Medium · Applied 2h ago          │  ││
│  │  │                              [Revert] [Details →]      │  ││
│  │  └────────────────────────────────────────────────────────┘  ││
│  │                                                              ││
│  │  ┌────────────────────────────────────────────────────────┐  ││
│  │  │ ○ Disable USB Selective Suspend     VALIDATED AVAILABLE│  ││
│  │  │   Prevents USB power saving that causes device drops   │  ││
│  │  │   Risk: Low · Impact: Low                              │  ││
│  │  │                                [Apply] [Details →]     │  ││
│  │  └────────────────────────────────────────────────────────┘  ││
│  │                                                              ││
│  │  ┌────────────────────────────────────────────────────────┐  ││
│  │  │ 🔒 Advanced CPU Parking Control     PREMIUM EXPERT_ONLY│ ││
│  │  │   Fine-tune CPU core parking behavior                  │  ││
│  │  │   Risk: Medium · Impact: High                          │  ││
│  │  │                           [Unlock Pro →] [Details →]   │  ││
│  │  └────────────────────────────────────────────────────────┘  ││
│  │                                                              ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌─── Category: Network ───────────────────────────────────────┐│
│  │  ...                                                         ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

**Page Header:**
- Title: "Optimizations" (`display`)
- Primary action: "Apply All Safe" dropdown button (applies only VALIDATED + Low Risk tweaks)
- Dropdown options: "Apply All Safe", "Apply All Validated", "Revert All" (danger)

**Filter Bar:**
- Tab-style filters: All | Active | Available | Reverted | Failed
- Each tab shows count badge
- Search input (right-aligned): filters by name/description
- Additional filter dropdown: Risk Level, Impact, Category, Status badges
- Expert mode toggle (if expert mode enabled in settings): shows/hides EXPERT_ONLY items

**Optimization List:**
- Grouped by category (Power Management, Network, Storage, Memory, GPU, Services, etc.)
- Category headers: `heading-sm`, `text-secondary`, collapsible (chevron icon)
- Each category shows count of items

**Optimization Item Card:**
- Full-width card within category group
- Left: selection checkbox (for bulk actions)
- Content:
  - Title (`heading-md`, `text-primary`)
  - Description (`body-sm`, `text-secondary`) — one line, truncated
  - Metadata line: Risk level (Low/Medium/High with color dot) · Impact level · Timestamp
  - Badges: up to 3 status badges (right-aligned on title line)
- Actions (right side):
  - If available: [Apply] secondary button
  - If active: [Revert] ghost button
  - If failed: [Retry] ghost button
  - If premium locked: [Unlock Pro →] premium button
  - Always: [Details →] ghost button → opens detail drawer

**Optimization Detail Drawer (420px, slides from right):**

```
┌──────────────────────────────────┐
│  ✕  Disable USB Selective Suspend│
│─────────────────────────────────│
│                                  │
│  Status: VALIDATED · ACTIVE      │
│  Applied: Jan 15, 2025 14:32    │
│  Applied by: Manual             │
│                                  │
│  ── Description ──               │
│  Prevents Windows from           │
│  suspending USB devices to save  │
│  power, which can cause input    │
│  device disconnections during    │
│  gaming sessions.                │
│                                  │
│  ── Rationale ──                 │
│  USB selective suspend saves     │
│  minimal power but introduces    │
│  latency and disconnection risk  │
│  for gaming peripherals.         │
│                                  │
│  ── Risk Assessment ──           │
│  Risk: Low                       │
│  Impact: Medium                  │
│  Reversible: Yes (instant)       │
│  Requires restart: No            │
│  Requires admin: Yes             │
│                                  │
│  ── Before / After ──            │
│  Registry: HKLM\SYSTEM\...      │
│  Before: 1 (enabled)            │
│  After:  0 (disabled)           │
│                                  │
│  ── Benchmark Evidence ──        │
│  USB latency: -12ms avg         │
│  Device reconnects: 0 (was 3)   │
│                                  │
│  ── History ──                   │
│  Jan 15 14:32 — Applied          │
│  Jan 14 09:15 — Reverted         │
│  Jan 13 22:00 — Applied          │
│                                  │
│  ─────────────────────────────── │
│              [Revert]  [Close]   │
└──────────────────────────────────┘
```

**Drawer Sections (collapsible):**
1. **Status & Metadata** — badges, timestamps, applied-by
2. **Description** — full explanation
3. **Rationale** — why this optimization matters
4. **Risk Assessment** — structured risk/impact/reversibility info
5. **Before / After** — technical values (registry, config) in `mono` font
6. **Benchmark Evidence** — linked benchmark data if available
7. **History** — timeline of apply/revert actions

**States:**
- **UNSUPPORTED items:** Shown with 50% opacity, "Not available on this system" message, no action buttons
- **EXPERT_ONLY items:** Hidden by default, shown when expert mode is on, with amber warning banner in drawer
- **PREMIUM items:** Lock icon overlay, content visible but actions replaced with upgrade CTA
- **FAILED items:** Red severity bar, error message in card, "Retry" and "View Error" actions
- **AI_APPLIED items:** Blue accent, "Applied by PulseBoost AI" attribution

---

### 5.3 Network

**Philosophy:** Elegant, protocol-aware network diagnostics. Not a network monitoring tool — a diagnostic and optimization surface for gaming and performance use cases.

**Layout:**

```
┌──────────────────────────────────────────────────────────────────┐
│  Network                                                         │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │ [Overview] [Diagnostics] [QoS] [Adapters]                   ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  === OVERVIEW TAB ===                                            │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  Connection Status                                           ││
│  │                                                              ││
│  │  Interface: Intel I225-V · Ethernet · 1 Gbps                ││
│  │  Latency: 14ms        Jitter: 2ms        Loss: 0.0%        ││
│  │  Download: 412 Mbps   Upload: 38 Mbps                      ││
│  │                                                              ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌───────────────────────────┐  ┌──────────────────────────────┐│
│  │  Latency (last 30 min)    │  │  Throughput (last 30 min)    ││
│  │                            │  │                              ││
│  │  [Refined line chart]      │  │  [Refined area chart]        ││
│  │                            │  │                              ││
│  └───────────────────────────┘  └──────────────────────────────┘│
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  Recommendations                                             ││
│  │  ● Disable Nagle's Algorithm — reduce micro-latency          ││
│  │  ● Enable RSS — improve multi-core packet processing         ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

**Tab Structure:**

**Tab 1: Overview**
- Connection status card (full width): adapter name, type, speed, key metrics (latency, jitter, loss, throughput)
- Two charts side by side: Latency over time, Throughput over time
- Charts: refined, minimal — 2px lines, subtle grid, no excessive decoration
- Recommendations card: top network-related optimizations

**Tab 2: Diagnostics**
- Run diagnostic button (primary CTA)
- Results appear as a structured report:
  - DNS resolution time
  - Route hop count
  - Packet loss test
  - TCP window scaling status
  - Nagle's algorithm status
  - Each result: metric name, value, status badge (Good/Warning/Poor)
- Historical diagnostic runs (collapsible list)

**Tab 3: QoS (Quality of Service)**
- Current QoS policy status
- Per-application bandwidth allocation (if supported)
- DSCP marking status
- UNSUPPORTED state: clean message explaining QoS requires specific NIC/driver support, with link to compatible hardware
- Premium features: advanced QoS rules (PREMIUM badge, upgrade CTA)

**Tab 4: Adapters**
- List of all network adapters
- Each adapter card:
  - Name, type (Ethernet/Wi-Fi/Virtual), status (Connected/Disconnected)
  - MAC address, IP address, DNS servers (`mono-sm`)
  - Driver version, driver date
  - Capabilities: RSS, TCP Offload, Jumbo Frames — each with supported/unsupported badge
- Click adapter → detail drawer with full properties

**Unsupported States:**
- Features requiring specific NIC capabilities show a clean "Not supported by [Adapter Name]" message
- Gray out controls, show informational tooltip explaining the requirement
- Never hide unsupported features — show them as informational

---

### 5.4 GPU

**Philosophy:** Vendor-aware GPU telemetry and advisory. Respects the boundary between what PulseBoost can control and what requires vendor tools or BIOS changes.

**Layout:**

```
┌──────────────────────────────────────────────────────────────────┐
│  GPU                                          [NVIDIA RTX 4070] │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │ [Telemetry] [Optimizations] [Driver] [Advisory]             ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  === TELEMETRY TAB ===                                           │
│                                                                  │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐   │
│  │ Core Clock  │ │ Memory     │ │ Temperature│ │ Power Draw │   │
│  │ 1,920 MHz  │ │ 8.2 / 12GB│ │ 62°C       │ │ 142W       │   │
│  │ [sparkline]│ │ [sparkline]│ │ [sparkline]│ │ [sparkline]│   │
│  └────────────┘ └────────────┘ └────────────┘ └────────────┘   │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  GPU Load (last 30 min)                                      ││
│  │  [Refined area chart — GPU utilization over time]            ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  Fan Speed & Thermal                                         ││
│  │  Fan 1: 1,240 RPM (45%)    Target: 65°C                    ││
│  │  Fan 2: 1,180 RPM (43%)    Throttle: None                  ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

**Page Header:**
- Title: "GPU" (`display`)
- GPU model badge (right-aligned): "[NVIDIA RTX 4070]" in `heading-sm` with vendor-colored accent

**Tab Structure:**

**Tab 1: Telemetry**
- Top row: 4 metric cards (equal width)
  - Each: metric name (`caption`), current value (`heading-lg`, `mono`), tiny sparkline (40px wide, 20px tall)
  - Core Clock, Memory Usage, Temperature, Power Draw
- Main chart: GPU Load over time (area chart, 30-minute window)
- Fan/Thermal card: fan speeds, target temp, throttle status

**Tab 2: Optimizations**
- GPU-specific optimizations (shader cache, power management mode, etc.)
- Same card pattern as Optimizations page but filtered to GPU category
- Vendor-specific tweaks clearly labeled (e.g., "NVIDIA Only" badge)

**Tab 3: Driver**
- Current driver version, date, type (Game Ready / Studio)
- Driver recommendation: "Up to date" or "Update available" with version comparison
- Driver history (collapsible): last 5 installed versions
- Note: PulseBoost does NOT install drivers — links to vendor download page
- Clean informational card explaining this boundary

**Tab 4: Advisory**
- BIOS-level settings that affect GPU performance
- Each item is ADVISORY ONLY — clearly marked "Requires BIOS change"
- Items: Resizable BAR, PCIe Gen settings, Above 4G Decoding
- Each shows: current status (if detectable), recommendation, how-to guidance
- Visual treatment: lighter card style, informational icon, no action buttons
- Clear header: "These settings require BIOS changes and cannot be modified by PulseBoost"

**Vendor Awareness:**
- NVIDIA: Show CUDA cores, RT cores, DLSS support
- AMD: Show compute units, ray accelerators, FSR support
- Intel: Show execution units, Xe cores, XeSS support
- Unknown/unsupported: Graceful fallback showing basic metrics only

**Multi-GPU:**
- GPU selector dropdown in page header if multiple GPUs detected
- Each GPU gets its own telemetry context

---

### 5.5 Audit Log

**Philosophy:** Elite transparency. Every change PulseBoost makes is logged, visible, and reversible. This page builds trust by showing users exactly what happened, when, and why.

**Layout:**

```
┌──────────────────────────────────────────────────────────────────┐
│  Audit Log                                    [Export ↓] [Clear]│
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │ [Timeline] [List]          🔍 Search    [Filters ▾]         ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  === TIMELINE VIEW ===                                           │
│                                                                  │
│  Today                                                           │
│  ──●── 14:32  Applied "High Performance Power Plan"              │
│  │            VALIDATED · ACTIVE · Manual                        │
│  │            Before: Balanced → After: High Performance         │
│  │                                          [Revert] [Details]  │
│  │                                                               │
│  ──●── 14:28  AI applied 3 network optimizations                 │
│  │            AI_APPLIED · ACTIVE · Adaptive Engine              │
│  │            Nagle's Algorithm, TCP Window, RSS                 │
│  │                                          [Revert All] [Details]│
│  │                                                               │
│  ──●── 12:15  Reverted "Disable Prefetch"                        │
│  │            REVERTED · Manual                                  │
│  │            Reason: User-initiated revert                      │
│  │                                                    [Details]  │
│  │                                                               │
│  Yesterday                                                       │
│  ──○── 22:00  Benchmark completed                                │
│  │            Score: 8,420 → 8,890 (+5.6%)                      │
│  │                                                    [Details]  │
│  │                                                               │
│  ──●── 18:30  Applied "Disable Windows Search Indexing"          │
│               VALIDATED · ACTIVE · Manual                        │
│               ...                                                │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

**View Toggle:**
- Timeline view (default): vertical timeline with connected dots
- List view: compact table format for power users

**Timeline View:**
- Vertical line (1px, `border-default`) connecting events
- Each event node: 10px circle, color-coded by type
  - Applied: `accent-primary` filled
  - Reverted: `status-neutral` filled
  - Failed: `status-error` filled
  - Benchmark: `status-info` outlined
  - AI action: `accent-primary` outlined
- Date headers: `heading-sm`, `text-tertiary`, sticky
- Event content:
  - Title (`heading-sm`, `text-primary`)
  - Badges (status badges)
  - Before/After summary (if applicable, `mono-sm`)
  - Action buttons: [Revert] (if active), [Details →]

**List View:**
- Table with columns: Time | Action | Target | Status | Source | Actions
- Sortable by any column
- Row click → opens detail drawer

**Filters:**
- Type: Applied, Reverted, Failed, Benchmark, AI Action, System Event
- Source: Manual, Adaptive Engine, Game Profile, Scheduled
- Date range picker
- Status badges filter

**Detail Drawer:**
- Full event details including:
  - Complete before/after values
  - Registry/config paths
  - Source attribution (who/what triggered)
  - Related events (e.g., "Part of batch apply on Jan 15")
  - Revert action (if applicable)
  - Error details (if failed)

**Export:**
- Export as JSON or CSV
- Date range selection for export
- Includes all metadata

---

### 5.6 Benchmark

**Philosophy:** Scientific proof page. Benchmarks exist to validate that optimizations actually work. Before/after comparison is the core interaction. No vanity metrics.

**Layout:**

```
┌──────────────────────────────────────────────────────────────────┐
│  Benchmark                                    [Run Benchmark ▶] │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │ [Latest Result] [History] [Compare]                         ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  === LATEST RESULT TAB ===                                       │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  Benchmark #47 — Jan 15, 2025 14:45                         ││
│  │  Duration: 4m 32s · Tweaks active: 14                       ││
│  │                                                              ││
│  │  ┌─────────────────────────────────────────────────────────┐ ││
│  │  │  VERDICT                                                │ ││
│  │  │                                                         │ ││
│  │  │  Overall Score: 8,890          Change: +5.6% ▲          │ ││
│  │  │  Previous:      8,420                                   │ ││
│  │  │                                                         │ ││
│  │  │  Verdict: Measurable Improvement                        │ ││
│  │  └─────────────────────────────────────────────────────────┘ ││
│  │                                                              ││
│  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐        ││
│  │  │ CPU Score     │ │ GPU Score     │ │ Memory Score │        ││
│  │  │ 4,210 (+3.2%)│ │ 3,890 (+8.1%)│ │ 790 (+2.4%) │        ││
│  │  └──────────────┘ └──────────────┘ └──────────────┘        ││
│  │                                                              ││
│  │  ── Tweak Attribution ──                                     ││
│  │  Power Plan change         → +2.1% CPU                      ││
│  │  Nagle's Algorithm disable → -8ms latency                   ││
│  │  GPU shader cache enable   → +4.2% GPU                      ││
│  │                                                              ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

**Page Header:**
- Title: "Benchmark" (`display`)
- Primary CTA: "Run Benchmark ▶" button
- Running state: button becomes progress indicator with cancel option

**Tab Structure:**

**Tab 1: Latest Result**
- Benchmark metadata: run number, date, duration, active tweak count
- **Verdict Card** (elevated, full width):
  - Overall score (`display` size, `text-primary`)
  - Change from previous: percentage with up/down arrow, color-coded
  - Previous score for reference
  - Verdict text: "Measurable Improvement" / "Marginal Change" / "No Significant Change" / "Regression Detected"
  - Verdict uses conservative language — never hype
- Sub-scores: 3 cards (CPU, GPU, Memory) each with score and delta
- **Tweak Attribution** section:
  - Which specific tweaks contributed to which improvements
  - Each attribution: tweak name → measured impact
  - This is the "proof" — connecting optimizations to results
- **Unsupported Metrics:**
  - If certain benchmarks can't run (e.g., no discrete GPU), show clean "Not available" state
  - Explain why and what hardware would be needed

**Tab 2: History**
- Table of all benchmark runs
- Columns: Run # | Date | Score | Delta | Tweaks Active | Duration
- Sparkline column showing score trend
- Click row → shows that run's full results

**Tab 3: Compare**
- Side-by-side comparison of two benchmark runs
- Dropdown selectors for "Run A" and "Run B"
- Comparison table: metric | Run A | Run B | Delta
- Visual diff bars showing improvement/regression per metric
- Tweak diff: what was different between the two runs

**Benchmark Running State:**
- Modal overlay (not blocking — can be minimized)
- Progress bar with current phase label
- Phases: "Preparing..." → "CPU Benchmark" → "GPU Benchmark" → "Memory Benchmark" → "Analyzing..."
- Estimated time remaining
- Cancel button (secondary, with confirmation)

**Benchmark Completion Summary (Flow):**
- Appears as a modal after benchmark completes
- Shows verdict card prominently
- "View Full Results" → navigates to Latest Result tab
- "Compare with Previous" → navigates to Compare tab
- "Dismiss" → closes modal

---

### 5.7 Game Profiles

**Philosophy:** Polished profile intelligence. Game profiles are curated optimization presets that activate when specific games launch. They should feel like a premium feature, not a simple toggle list.

**Layout:**

```
┌──────────────────────────────────────────────────────────────────┐
│  Game Profiles                         [Import] [Create Profile]│
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │ [My Profiles] [Recommended] [Community]                     ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  === MY PROFILES TAB ===                                         │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  ┌──────────────────────────────────────────────────────┐    ││
│  │  │  [Game Icon]  Cyberpunk 2077              ACTIVE ●   │    ││
│  │  │               12 tweaks · Last used: 2h ago          │    ││
│  │  │               Stability: Excellent · Score: +8.2%    │    ││
│  │  │                    [Edit] [Benchmark] [Deactivate]   │    ││
│  │  └──────────────────────────────────────────────────────┘    ││
│  │                                                              ││
│  │  ┌──────────────────────────────────────────────────────┐    ││
│  │  │  [Game Icon]  Valorant                               │    ││
│  │  │               8 tweaks · Last used: 1d ago           │    ││
│  │  │               Stability: Good · Score: +3.1%         │    ││
│  │  │                         [Edit] [Benchmark] [Activate]│    ││
│  │  └──────────────────────────────────────────────────────┘    ││
│  │                                                              ││
│  │  ┌──────────────────────────────────────────────────────┐    ││
│  │  │  [Game Icon]  Flight Simulator 2024    PREMIUM 🔒    │    ││
│  │  │               AI-optimized · 18 tweaks               │    ││
│  │  │               Requires Pro plan                      │    ││
│  │  │                                       [Unlock Pro →] │    ││
│  │  └──────────────────────────────────────────────────────┘    ││
│  │                                                              ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

**Tab Structure:**

**Tab 1: My Profiles**
- List of user-created and applied profiles
- Each profile card:
  - Game icon (if available, fallback: first letter avatar)
  - Game name (`heading-md`)
  - Metadata: tweak count, last used, stability rating, benchmark score delta
  - Status: ACTIVE (green dot) or inactive
  - Actions: Edit, Benchmark (run benchmark with this profile), Activate/Deactivate
  - Premium-locked profiles: shown with lock overlay and upgrade CTA
- Empty state: "No profiles yet. Create one or browse recommendations."

**Tab 2: Recommended**
- PulseBoost-curated profiles for popular games
- Each shows: game name, tweak count, expected improvement range, compatibility
- "Apply" button → creates a copy in My Profiles
- PREMIUM profiles clearly marked

**Tab 3: Community** (Premium feature)
- Community-shared profiles
- Rating system (thumbs up/down, not stars)
- Download count
- Compatibility badges
- "Import" → adds to My Profiles
- Locked for free tier with upgrade CTA

**Profile Detail/Edit View (navigates to sub-page or wide drawer):**
- Profile name and game association
- List of included tweaks (same card pattern as Optimizations)
- Each tweak can be toggled on/off within the profile
- Tweak order (priority)
- Auto-activate settings: "Activate when [game executable] launches"
- Benchmark linkage: "Last benchmark with this profile: #47 — Score: 8,890"
- History: when profile was created, modified, activated, deactivated
- Export button → exports as .pulseboost file
- Delete button (danger, with confirmation)

**Import/Export Flow:**
- Import: file picker → validation → preview of tweaks → confirm import
- Export: select profile → choose format → download
- Formats: .pulseboost (proprietary JSON), .json (open)

---

### 5.8 Trust Center

**Philosophy:** The safety headquarters. This page exists to build and maintain user trust. It shows that PulseBoost is transparent, reversible, and safe. It's the "I can always undo everything" guarantee.

**Layout:**

```
┌──────────────────────────────────────────────────────────────────┐
│  Trust Center                                                    │
│  Your system's safety posture and recovery readiness             │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  RECOVERY READINESS                                          ││
│  │                                                              ││
│  │  [Shield Icon]  System Recovery: Ready ✓                     ││
│  │                                                              ││
│  │  Snapshot age: 2h 14m                                        ││
│  │  Snapshot size: 4.2 MB                                       ││
│  │  Active tweaks: 14 (all reversible)                          ││
│  │  Non-reversible changes: 0                                   ││
│  │                                                              ││
│  │  [Create Snapshot]  [Revert All Changes]                     ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌───────────────────────────┐  ┌──────────────────────────────┐│
│  │  SAFEGUARDS               │  │  PROTECTED PROCESSES          ││
│  │                            │  │                              ││
│  │  ✓ Auto-snapshot before   │  │  Windows Update: Protected   ││
│  │    batch operations        │  │  Defender: Protected          ││
│  │  ✓ Revert on failure      │  │  Firewall: Protected          ││
│  │  ✓ Admin elevation prompt │  │  BitLocker: Protected         ││
│  │  ✓ Expert mode gating     │  │                              ││
│  │  ○ Dry-run by default     │  │  PulseBoost will never       ││
│  │                            │  │  modify these services.      ││
│  │  [Configure →]            │  │                              ││
│  └───────────────────────────┘  └──────────────────────────────┘│
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  LIMITATIONS & TRANSPARENCY                                  ││
│  │                                                              ││
│  │  PulseBoost modifies Windows registry, services, and        ││
│  │  power settings. It does NOT modify:                         ││
│  │  • BIOS/UEFI settings                                       ││
│  │  • GPU firmware or vBIOS                                     ││
│  │  • Hardware voltage or clock speeds                          ││
│  │  • Third-party application settings                          ││
│  │  • Windows core system files                                 ││
│  │                                                              ││
│  │  All changes are logged in the Audit Log.                    ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  EXPERT MODE                                    [Disabled]   ││
│  │                                                              ││
│  │  Expert mode unlocks advanced optimizations that carry       ││
│  │  higher risk. These tweaks are hidden by default.            ││
│  │                                                              ││
│  │  When enabled:                                               ││
│  │  • EXPERT_ONLY tweaks become visible                         ││
│  │  • Additional confirmation steps for risky actions           ││
│  │  • Dry-run mode available for testing                        ││
│  │                                                              ││
│  │  [Enable Expert Mode]                                        ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

**Sections:**

**1. Recovery Readiness (hero card, full width):**
- Large shield icon with status color
- Recovery status: "Ready" (green), "Stale Snapshot" (amber), "Not Available" (red)
- Key metrics: snapshot age, size, active tweak count, non-reversible count
- Actions: [Create Snapshot] (secondary), [Revert All Changes] (danger — requires confirmation)

**2. Safeguards (left column):**
- Checklist of safety features with toggle states
- Each safeguard: checkbox/toggle + label + brief description
- "Configure →" link → opens Settings > Safety section

**3. Protected Processes (right column):**
- List of Windows services PulseBoost will never touch
- Each with "Protected" badge
- Informational — no actions needed
- Builds trust by showing boundaries

**4. Limitations & Transparency (full width):**
- Plain-English explanation of what PulseBoost does and doesn't do
- Bulleted list of things PulseBoost will never modify
- Link to Audit Log

**5. Expert Mode (full width):**
- Current state toggle (prominent)
- Explanation of what expert mode unlocks
- Warning about increased risk
- Enable requires confirmation dialog

---

### 5.9 Settings

**Philosophy:** Commercial desktop settings — organized, complete, but not overwhelming. Progressive disclosure keeps it clean.

**Layout:**

```
┌──────────────────────────────────────────────────────────────────┐
│  Settings                                                        │
│                                                                  │
│  ┌──────────┐ ┌────────────────────────────────────────────────┐│
│  │ SECTIONS  │ │                                                ││
│  │           │ │  === GENERAL ===                                ││
│  │ General   │ │                                                ││
│  │ Session   │ │  Start with Windows          [Toggle: On]      ││
│  │ Performance│ │  Minimize to tray            [Toggle: On]      ││
│  │ Safety    │ │  Check for updates            [Toggle: On]      ││
│  │ Expert    │ │  Update channel         [Dropdown: Stable ▾]   ││
│  │ Data      │ │  Language               [Dropdown: English ▾]  ││
│  │ Account   │ │  Theme                  [Dropdown: Dark ▾]     ││
│  │           │ │                                                ││
│  │           │ │  ── Notifications ──                            ││
│  │           │ │  Optimization alerts          [Toggle: On]      ││
│  │           │ │  Benchmark completion          [Toggle: On]      ││
│  │           │ │  Recovery warnings             [Toggle: On]      ││
│  │           │ │                                                ││
│  └──────────┘ └────────────────────────────────────────────────┘│
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

**Navigation:** Left sidebar within settings (not tabs — settings pages are too long for tabs)

**Sections:**

**General:**
- Startup behavior (start with Windows, minimize to tray)
- Update settings (auto-update, update channel: Stable/Beta)
- Language
- Theme (Dark only for now — noted as "More themes coming")
- Notification preferences

**Session:**
- Session duration tracking (on/off)
- Auto-optimization on session start
- Session summary on close
- Idle detection settings

**Performance:**
- Telemetry polling interval
- Chart data retention period
- Background monitoring (on/off)
- Resource usage limits for PulseBoost itself

**Safety:**
- Auto-snapshot before batch operations
- Revert on failure
- Admin elevation prompt behavior
- Dry-run by default (toggle)
- Maximum simultaneous active tweaks

**Expert:**
- Expert mode toggle (mirrors Trust Center)
- Show experimental tweaks
- Verbose logging
- Debug overlay (for PulseBoost itself)

**Data:**
- Export all data (JSON)
- Export settings
- Import settings
- Clear benchmark history
- Clear audit log
- Reset all settings to defaults (danger)
- Uninstall cleanup (removes all PulseBoost data)

**Account:**
- Quick link to Account page
- Sign in / Sign out
- License key entry (for offline activation)

**Setting Item Anatomy:**
- Label (`body`, `text-primary`) + description (`body-sm`, `text-secondary`)
- Control right-aligned: toggle, dropdown, or input
- Grouped by sub-section with `heading-sm` headers
- 12px vertical spacing between items
- Sub-sections separated by `border-subtle` divider

---

### 5.10 Account / Plan / Licensing

**Philosophy:** Clean identity and entitlement management. Shows the user their plan, what they have access to, and tasteful upgrade paths.

**Layout:**

```
┌──────────────────────────────────────────────────────────────────┐
│  Account                                                         │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  [Avatar]  John Doe                                          ││
│  │            john@example.com                                  ││
│  │            Member since Jan 2024                             ││
│  │                                          [Edit] [Sign Out]   ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  CURRENT PLAN                                                ││
│  │                                                              ││
│  │  ┌─────────────────────────────────────────────────────────┐ ││
│  │  │  PulseBoost Pro                              ACTIVE ●   │ ││
│  │  │                                                         │ ││
│  │  │  Renews: Feb 15, 2025                                   │ ││
│  │  │  Billing: Monthly · $9.99/mo                            │ ││
│  │  │                                                         │ ││
│  │  │  [Manage Subscription]  [View Invoice History]          │ ││
│  │  └─────────────────────────────────────────────────────────┘ ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  FEATURE ACCESS                                              ││
│  │                                                              ││
│  │  ✓ Core optimizations (unlimited)                            ││
│  │  ✓ Advanced optimizations                                    ││
│  │  ✓ AI adaptive engine                                        ││
│  │  ✓ Game profiles (unlimited)                                 ││
│  │  ✓ Community profiles                                        ││
│  │  ✓ Priority support                                          ││
│  │  ○ Enterprise API access           [Enterprise →]            ││
│  │  ○ Multi-device management          [Enterprise →]            ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  DEVICES                                                     ││
│  │                                                              ││
│  │  ● This PC — DESKTOP-ABC123          Active                  ││
│  │  ○ Gaming Laptop — LAPTOP-XYZ789     Last seen: 3d ago      ││
│  │                                                              ││
│  │  2 of 3 devices used                                         ││
│  │                                          [Manage Devices]    ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  LICENSE                                                     ││
│  │                                                              ││
│  │  License key: XXXX-XXXX-XXXX-XXXX    [Copy] [Deactivate]   ││
│  │  Activation date: Jan 15, 2024                               ││
│  │  License type: Personal                                      ││
│  └──────────────────────────────────────────────────────────────┘│
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

**Sections:**

**1. Identity Card (top):**
- Avatar (48px circle), name, email, member since date
- Edit profile, Sign out actions

**2. Current Plan:**
- Plan name with ACTIVE badge
- Renewal date, billing cycle, price
- Manage subscription link (opens external billing portal)
- Invoice history link

**3. Feature Access:**
- Checklist of all features with included (✓) / not included (○)
- Not-included features show which plan unlocks them
- Clean, scannable format

**4. Devices:**
- List of activated devices
- Current device highlighted
- Device count vs. limit
- Manage devices action

**5. License:**
- License key (masked, with reveal/copy)
- Activation details
- Deactivate action (danger, with confirmation)

**Free Tier State:**
- Plan card shows "PulseBoost Free" with feature limitations listed
- Prominent but tasteful "Upgrade to Pro" card below plan info
- Feature access shows locked items with "Pro" / "Enterprise" badges
- No aggressive upselling — just clear information

**Signed-Out State:**
- Identity card replaced with sign-in prompt
- Plan shows "Not signed in — some features require an account"
- Sign in button (primary) + "Continue without account" (ghost)
- Offline/license-key activation option visible

---

## 6. Flow Specifications

### 6.1 First Launch

**Trigger:** Application opened for the first time after installation.

**Flow:**

```
Step 1: Welcome
┌──────────────────────────────────────────────────┐
│                                                  │
│         [PulseBoost Logo — large]                │
│                                                  │
│         Welcome to PulseBoost                    │
│                                                  │
│         Precision optimization for               │
│         your Windows PC.                         │
│                                                  │
│                                                  │
│                    [Get Started]                  │
│                                                  │
└──────────────────────────────────────────────────┘

Step 2: System Scan
┌──────────────────────────────────────────────────┐
│                                                  │
│         Analyzing your system...                 │
│                                                  │
│         [Progress bar]                           │
│                                                  │
│         ✓ Hardware detected                      │
│         ✓ OS version confirmed                   │
│         ● Scanning for optimizations...          │
│         ○ Checking compatibility                 │
│                                                  │
│                                                  │
└──────────────────────────────────────────────────┘

Step 3: Results Summary
┌──────────────────────────────────────────────────┐
│                                                  │
│         System Analysis Complete                 │
│                                                  │
│         CPU: AMD Ryzen 7 5800X                   │
│         GPU: NVIDIA RTX 4070                     │
│         RAM: 32 GB DDR4                          │
│         OS:  Windows 11 23H2                     │
│                                                  │
│         12 optimizations available               │
│         3 recommended immediately                │
│                                                  │
│         [View Recommendations]  [Skip to Dashboard]│
│                                                  │
└──────────────────────────────────────────────────┘

Step 4: Account (Optional)
┌──────────────────────────────────────────────────┐
│                                                  │
│         Sign in or create an account             │
│         to unlock all features.                  │
│                                                  │
│         [Sign In]  [Create Account]              │
│                                                  │
│         [Continue without account →]             │
│                                                  │
└──────────────────────────────────────────────────┘
```

**Rules:**
- Total flow: 4 steps maximum
- Each step fits in a single centered card (480px wide)
- No sidebar navigation during onboarding
- Progress dots at bottom (4 dots)
- Can skip at any step (except system scan which auto-advances)
- System scan is real — actually detects hardware and finds optimizations
- Account step is optional and clearly skippable

### 6.2 Admin Elevation Prompt

**Trigger:** User attempts an action requiring administrator privileges.

```
┌──────────────────────────────────────────────────┐
│                                                  │
│  [Shield Icon — amber]                           │
│                                                  │
│  Administrator Access Required                   │
│                                                  │
│  "Disable USB Selective Suspend" requires         │
│  elevated privileges to modify the Windows        │
│  registry.                                        │
│                                                  │
│  PulseBoost will request administrator access     │
│  through Windows UAC.                             │
│                                                  │
│  ☐ Don't ask again for this session              │
│                                                  │
│              [Cancel]  [Continue as Admin]        │
│                                                  │
└──────────────────────────────────────────────────┘
```

**Rules:**
- Modal dialog, 440px wide
- Amber shield icon (not red — this is expected, not dangerous)
- Clear explanation of WHY admin is needed
- Specific action named
- "Don't ask again" checkbox (per-session only)
- Cancel is secondary (left), Continue is primary (right)
- After clicking Continue, Windows UAC prompt appears (outside PulseBoost control)

### 6.3 Risky Action Confirmation

**Trigger:** User attempts to apply a Medium or High risk optimization, or any EXPERT_ONLY tweak.

```
┌──────────────────────────────────────────────────┐
│                                                  │
│  [Warning Icon — amber/red based on risk]        │
│                                                  │
│  Confirm: Apply "Advanced CPU Parking Control"   │
│                                                  │
│  Risk Level: High                                │
│  This optimization modifies CPU core parking     │
│  behavior which may cause instability on some    │
│  systems.                                        │
│                                                  │
│  ── What will change ──                          │
│  Registry: HKLM\SYSTEM\CurrentControlSet\...     │
│  Current: 100 (default)                          │
│  New: 0 (all cores unparked)                     │
│                                                  │
│  ✓ A recovery snapshot will be created           │
│  ✓ This change can be reverted instantly         │
│                                                  │
│  ☐ I understand the risk                         │
│                                                  │
│           [Cancel]  [Apply Optimization]          │
│                                                  │
└──────────────────────────────────────────────────┘
```

**Rules:**
- Modal, 440px wide
- Icon color matches risk level (amber for Medium, red for High)
- Names the specific optimization
- Shows risk level prominently
- Plain-English explanation of what will change
- Technical before/after in `mono` font
- Safety reassurances (snapshot, reversibility)
- Checkbox acknowledgment required for High risk (button disabled until checked)
- Medium risk: checkbox optional, button always enabled
- Cancel left, Apply right

### 6.4 Revert Confirmation

**Trigger:** User clicks Revert on an active optimization.

```
┌──────────────────────────────────────────────────┐
│                                                  │
│  Revert "High Performance Power Plan"?           │
│                                                  │
│  This will restore the previous setting:         │
│  Power Plan: Balanced                            │
│                                                  │
│  Applied: 2h ago · Source: Manual                │
│                                                  │
│              [Cancel]  [Revert]                   │
│                                                  │
└──────────────────────────────────────────────────┘
```

**Rules:**
- Simpler than risky action confirmation (reverting is inherently safe)
- Shows what will be restored
- No checkbox required
- Revert button uses secondary style (not danger — reverting is safe)

### 6.5 Revert All Confirmation

**Trigger:** User clicks "Revert All Changes" in Trust Center.

```
┌──────────────────────────────────────────────────┐
│                                                  │
│  [Warning Icon — red]                            │
│                                                  │
│  Revert All Changes?                             │
│                                                  │
│  This will revert all 14 active optimizations    │
│  to their original values.                       │
│                                                  │
│  This action cannot be undone.                   │
│                                                  │
│  Type "REVERT ALL" to confirm:                   │
│  [________________________]                      │
│                                                  │
│              [Cancel]  [Revert All]              │
│                                                  │
└──────────────────────────────────────────────────┘
```

**Rules:**
- Requires typing "REVERT ALL" to enable the button
- Red warning icon
- Danger-filled button style
- Clear count of affected optimizations

### 6.6 Unsupported Capability State

**Pattern used across multiple pages when a feature isn't available on the user's hardware/OS.**

```
┌──────────────────────────────────────────────────┐
│                                                  │
│  [Muted icon — 32px]                             │
│                                                  │
│  QoS Configuration Not Available                 │
│                                                  │
│  Your network adapter (Realtek RTL8111) does     │
│  not support advanced QoS packet scheduling.     │
│                                                  │
│  Adapters with Intel I225-V or Killer E3100      │
│  chipsets support this feature.                  │
│                                                  │
│  [Learn More →]                                  │
│                                                  │
└──────────────────────────────────────────────────┘
```

**Rules:**
- Centered within the content area where the feature would normally appear
- Muted icon (not error — this isn't a failure)
- Clear explanation of WHY it's unsupported
- Specific hardware/software requirement mentioned
- Optional "Learn More" link
- Never hide the section entirely — show it as informational
- UNSUPPORTED badge on any related items

### 6.7 Crash Recovery

**Trigger:** PulseBoost detects it was not shut down cleanly, or system instability after optimization.

```
Step 1: Recovery Detection
┌──────────────────────────────────────────────────┐
│                                                  │
│  [Shield Icon — red, pulsing]                    │
│                                                  │
│  Recovery Mode                                   │
│                                                  │
│  PulseBoost detected an unexpected shutdown.     │
│  Your last session had 14 active optimizations.  │
│                                                  │
│  We recommend reviewing recent changes.          │
│                                                  │
│  [Review Changes]  [Revert All]  [Dismiss]       │
│                                                  │
└──────────────────────────────────────────────────┘

Step 2: Review (if selected)
→ Opens Audit Log filtered to last session's changes
→ Each item has prominent [Revert] button
→ Banner at top: "Recovery Mode — Review and revert any problematic changes"
```

**Rules:**
- Appears as a modal on app launch after crash
- Three clear options: review (recommended), revert all (safe), dismiss (advanced)
- If user selects Review, navigates to filtered Audit Log
- Recovery Mode badge appears in Dashboard hero until dismissed
- Recovery Mode banner appears at top of all pages until resolved

### 6.8 Benchmark Completion Summary

**(Covered in Benchmark section — modal overlay after benchmark completes)**

### 6.9 Profile Import/Export

**Import Flow:**
1. Click "Import" → native file picker opens
2. File validation (show spinner briefly)
3. Preview modal:
   - Profile name, game, tweak count
   - List of included tweaks with compatibility check
   - Incompatible tweaks marked with UNSUPPORTED badge
   - "Import X of Y tweaks" summary
4. [Cancel] [Import Profile]
5. Success toast: "Profile imported successfully"

**Export Flow:**
1. Click "Export" on a profile
2. Format selection dropdown: .pulseboost / .json
3. Native save dialog opens
4. Success toast: "Profile exported"

### 6.10 Locked Premium Feature Prompt

**Trigger:** User interacts with a PREMIUM or LOCKED feature on the free tier.

```
┌──────────────────────────────────────────────────┐
│                                                  │
│  [PulseBoost Pro icon — purple accent]           │
│                                                  │
│  Unlock Advanced CPU Parking Control             │
│                                                  │
│  This optimization is available with             │
│  PulseBoost Pro.                                 │
│                                                  │
│  Pro includes:                                   │
│  • Advanced optimizations (28 additional)        │
│  • AI adaptive engine                            │
│  • Game profile intelligence                     │
│  • Priority support                              │
│                                                  │
│  Starting at $9.99/month                         │
│                                                  │
│         [Maybe Later]  [Upgrade to Pro]          │
│                                                  │
└──────────────────────────────────────────────────┘
```

**Rules:**
- Modal, 440px wide
- Purple accent (premium color)
- Names the specific feature that triggered the prompt
- Brief feature list (4-5 items max)
- Price shown clearly
- "Maybe Later" is secondary (left), "Upgrade to Pro" is premium-styled button (right)
- Appears maximum once per feature per session (don't nag)
- Never blocks access to free features

### 6.11 Upgrade Prompt (Contextual)

**Appears inline (not modal) in relevant contexts:**

```
┌──────────────────────────────────────────────────┐
│  ✦ Unlock 28 more optimizations with Pro         │
│                                    [Learn More →]│
└──────────────────────────────────────────────────┘
```

**Rules:**
- Slim inline banner (40px height)
- `status-premium-muted` background
- `status-premium` text and icon
- Appears at most once per page
- Positioned after the last free item in a list, or at bottom of a section
- "Learn More" navigates to Account page
- Dismissible (×) — stays dismissed for the session

### 6.12 Toast / Notification System

**Position:** Bottom-right corner, 16px from window edges

**Variants:**

| Type | Left Bar Color | Icon | Auto-dismiss |
|---|---|---|---|
| Success | `status-success` | CheckCircle | 5 seconds |
| Info | `status-info` | Info | 5 seconds |
| Warning | `status-warning` | AlertTriangle | 8 seconds |
| Error | `status-error` | XCircle | Persistent (manual dismiss) |

**Anatomy:**
```
┌─────────────────────────────────────┐
│█ [Icon]  Optimization Applied        ✕│
│█         Power plan set to High      │
│█         Performance.          [Undo]│
└─────────────────────────────────────┘
```

- Width: 360px
- Left color bar: 3px
- Icon: 16px, color-matched
- Title: `heading-sm`, `text-primary`
- Description: `body-sm`, `text-secondary` (optional)
- Action link: "Undo" / "View" / "Details" (optional)
- Dismiss: × button (top-right)
- Stack: max 3 visible, newest on top, older ones slide down
- Animation: slide in from right (200ms), fade out (150ms)

### 6.13 Signed-Out vs Signed-In States

**Signed-In:**
- Sidebar account pill shows avatar + name + plan badge
- Account page shows full profile
- All plan-appropriate features unlocked
- Session data synced to account

**Signed-Out:**
- Sidebar account pill shows "Sign In" with user-plus icon
- Account page shows sign-in form
- Free tier features available
- Premium features show lock state
- Data stored locally only
- Subtle banner on Dashboard: "Sign in to sync your optimizations across devices"

**Transition:**
- Sign in: modal overlay with email/password form + OAuth options
- Sign out: confirmation dialog ("Your local data will be preserved")
- Session token expiry: toast notification + automatic redirect to sign-in

---

## 7. Monetization Patterns

### 7.1 Tier Structure

| Feature | Free | Pro ($9.99/mo) | Enterprise (Custom) |
|---|---|---|---|
| Core optimizations | ✓ (limited set) | ✓ (full set) | ✓ (full set) |
| Advanced optimizations | ✗ | ✓ | ✓ |
| AI adaptive engine | ✗ | ✓ | ✓ |
| Benchmark | Basic | Full + comparison | Full + comparison |
| Game profiles | 3 max | Unlimited | Unlimited |
| Community profiles | ✗ | ✓ | ✓ |
| Audit log | 7 days | Unlimited | Unlimited + export |
| Priority support | ✗ | ✓ | ✓ + dedicated |
| Multi-device | 1 device | 3 devices | Unlimited |
| API access | ✗ | ✗ | ✓ |
| Centralized management | ✗ | ✗ | ✓ |

### 7.2 Monetization Touchpoints

| Location | Treatment | Frequency |
|---|---|---|
| Locked optimization cards | Lock icon + "Unlock Pro →" button | Always visible |
| Game Profiles limit reached | Inline banner: "Create unlimited profiles with Pro" | When limit hit |
| Benchmark comparison tab | "Compare runs with Pro" overlay | Always on free tier |
| Community profiles tab | "Access community profiles with Pro" | Always on free tier |
| Audit log 7-day limit | "View full history with Pro" message after 7 days | When scrolling past limit |
| Dashboard recommendations | PREMIUM badge on locked recommendations | Always visible |
| Inline upgrade banner | Slim contextual banner (max 1 per page) | Once per page per session |
| Account page | Plan comparison and upgrade CTA | Always |

### 7.3 Monetization Rules

1. **Never block free functionality** — free features always work fully
2. **Never nag** — max 1 upgrade prompt per feature per session
3. **Always show value first** — user sees what the feature does before seeing the lock
4. **Price transparency** — always show price when showing upgrade CTA
5. **No dark patterns** — dismiss buttons are always visible and clearly labeled
6. **Graceful degradation** — if subscription lapses, features degrade to free tier (no data loss)
7. **Offline grace period** — 7 days of Pro features available offline after last sync

---

## 8. Microcopy Guidelines

### 8.1 Voice & Tone

| Attribute | Do | Don't |
|---|---|---|
| **Concise** | "Applied successfully" | "The optimization has been successfully applied to your system!" |
| **Technical** | "Disables USB selective suspend" | "Makes your USB stuff work better" |
| **Mature** | "This may cause instability" | "⚠️ DANGER! This could BREAK your PC!" |
| **Honest** | "Marginal improvement detected" | "MASSIVE PERFORMANCE BOOST!" |
| **Plain** | "Requires administrator access" | "Elevated privilege escalation required" |

### 8.2 Specific Patterns

**Action Buttons:**
- Apply / Revert / Retry / Export / Import (verb only)
- "View Details" not "Click here to see more"
- "Upgrade to Pro" not "Go Premium Now!"

**Status Messages:**
- "Optimization applied" (not "Success!")
- "Revert complete" (not "Successfully reverted!")
- "Benchmark running — estimated 4 minutes" (not "Please wait...")
- "3 optimizations available" (not "3 NEW optimizations waiting for you!")

**Error Messages:**
- "Failed to apply: insufficient permissions" (specific)
- "Network diagnostic timed out — check your connection" (actionable)
- "This optimization is not compatible with Windows 10 21H1" (precise)

**Warnings:**
- "This optimization carries higher risk on laptops" (contextual)
- "Reverting will restore your previous power plan" (consequence-clear)
- "Expert mode enables advanced tweaks that may affect system stability" (honest)

**Empty States:**
- "No benchmark results yet. Run your first benchmark to see how your system performs."
- "No active optimizations. Browse available tweaks to get started."
- "No game profiles created. Create one or browse recommendations."

### 8.3 Forbidden Language

- "Boost" / "Turbo" / "Ultra" / "Max" in UI copy (the product name is enough)
- "Amazing" / "Incredible" / "Blazing fast"
- "Click here"
- "Please" (in button labels)
- "Are you sure?" (be specific about what will happen)
- Exclamation marks in status messages
- ALL CAPS for emphasis (use badges instead)

---

## 9. Interaction & Motion Principles

### 9.1 Motion Budget

PulseBoost uses **minimal, purposeful motion**. Every animation must serve a functional purpose.

| Animation | Duration | Easing | Purpose |
|---|---|---|---|
| Page transition | 150ms | ease-out | Orient user to new context |
| Drawer slide-in | 200ms | ease-out | Reveal detail panel |
| Drawer slide-out | 150ms | ease-in | Dismiss detail panel |
| Modal appear | 150ms | ease-out | Focus attention |
| Modal dismiss | 100ms | ease-in | Return focus |
| Toast slide-in | 200ms | ease-out | Announce notification |
| Toast fade-out | 150ms | ease-in | Dismiss notification |
| Collapse/expand | 200ms | ease-in-out | Reveal/hide content |
| Tab underline | 150ms | ease-in-out | Indicate active tab |
| Hover state | 100ms | ease-out | Interactive feedback |
| Skeleton pulse | 800ms | ease-in-out (loop) | Loading indication |
| Progress bar | linear | — | Show progress |
| Health ring | 600ms | ease-out | Score reveal on load |

### 9.2 Motion Rules

1. **No decorative animation** — every motion serves navigation, feedback, or orientation
2. **No bounce effects** — ease-out for entrances, ease-in for exits
3. **No parallax or scroll-linked animation**
4. **No animated backgrounds or ambient effects**
5. **Respect prefers-reduced-motion** — disable all non-essential animation
6. **Charts animate on first render only** — no continuous animation
7. **Loading states use skeleton, not spinners** (except inline button loading)

### 9.3 Interaction Patterns

**Hover:** 
- Interactive elements only (buttons, links, clickable cards, table rows)
- Subtle background color shift (`bg-surface-hover`)
- 100ms transition
- Never on static content

**Focus:**
- 2px `accent-primary` ring, 2px offset
- Visible on keyboard navigation
- Hidden on mouse click (`:focus-visible` only)

**Active/Pressed:**
- Slight scale reduction (0.98) on buttons
- `bg-surface-active` on interactive surfaces
- 50ms transition

**Drag:**
- Not used in PulseBoost (no drag-and-drop interfaces)

**Right-Click:**
- No custom context menus — use native OS context menu

---

## 10. Accessibility & Keyboard Navigation

### 10.1 Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+K` | Open command palette |
| `Ctrl+,` | Open settings |
| `Ctrl+Z` | Undo last action (revert last optimization) |
| `Ctrl+Shift+Z` | Redo last reverted action |
| `Escape` | Close modal/drawer/palette |
| `Tab` | Navigate forward through interactive elements |
| `Shift+Tab` | Navigate backward |
| `Enter` | Activate focused element |
| `Space` | Toggle focused switch/checkbox |
| `Arrow keys` | Navigate within lists, tabs, menus |
| `1-9` | Navigate to sidebar items (when sidebar focused) |

### 10.2 Screen Reader Support

- All interactive elements have descriptive `aria-label`
- Status badges include screen-reader text (e.g., "Status: Validated, Active")
- Charts include `aria-description` with data summary
- Modals trap focus and announce title
- Toasts are announced via `aria-live="polite"` (info) or `aria-live="assertive"` (error)
- Page transitions announce new page title

### 10.3 Color Contrast

- All text meets WCAG AA contrast ratio (4.5:1 for body, 3:1 for large text)
- Status colors are never the sole indicator — always paired with text labels or icons
- Focus rings are visible against all surface colors

---

## Appendix A: Screen Inventory

| # | Screen | Primary Purpose | Key Actions |
|---|---|---|---|
| 1 | Dashboard | System health at a glance | View recommendations, check trust posture |
| 2 | Optimizations | Manage all tweaks | Apply, revert, filter, view details |
| 3 | Network | Network diagnostics | Run diagnostics, view adapters, configure QoS |
| 4 | GPU | GPU telemetry & advisory | Monitor, optimize, view driver info |
| 5 | Audit Log | Transparency & history | Search, filter, revert, export |
| 6 | Benchmark | Performance proof | Run benchmark, compare, view attribution |
| 7 | Game Profiles | Profile management | Create, edit, import/export, activate |
| 8 | Trust Center | Safety & recovery | Create snapshot, revert all, configure safeguards |
| 9 | Settings | App configuration | Toggle settings, manage data, configure safety |
| 10 | Account | Identity & licensing | Manage plan, view features, manage devices |

## Appendix B: State Matrix

Every screen must handle these states:

| State | Treatment |
|---|---|
| **Loading** | Skeleton matching content layout |
| **Empty** | Centered empty state with icon + message + optional CTA |
| **Error** | Inline error card with retry action |
| **Signed out** | Feature works if possible; sign-in prompt where account required |
| **Free tier** | All features visible; locked ones show premium badge + upgrade CTA |
| **Pro tier** | All features unlocked except Enterprise |
| **Enterprise** | All features unlocked |
| **Offline** | Banner: "Offline — some features may be limited" |
| **Recovery mode** | Red banner across all pages; recovery CTA prominent |
| **Expert mode on** | Expert-only items visible with amber indicators |
| **Expert mode off** | Expert-only items hidden |
| **Unsupported hardware** | Clean informational state per feature |

---

*End of PulseBoost Design Specification*
*Version 1.0 — Generated for premium desktop application UI*

```
```tailwind.config.js

export default {
  theme: {
    extend: {
      colors: {
        app: '#0f1117',
        surface: {
          DEFAULT: '#1a1d27',
          elevated: '#1f2231',
          hover: '#252838',
          active: '#2a2e40',
          sunken: '#13151c',
        },
        nav: {
          DEFAULT: '#14161f',
          active: '#1c1f2d',
        },
        border: {
          default: '#2a2d3a',
          subtle: '#1f2231',
          strong: '#3a3d4a',
        },
        txt: {
          primary: '#e8eaf0',
          secondary: '#9ca0b0',
          tertiary: '#6b7084',
          disabled: '#4a4e60',
        },
        accent: {
          DEFAULT: '#6366f1',
          hover: '#7577f5',
          muted: 'rgba(99,102,241,0.12)',
        },
        success: {
          DEFAULT: '#34d399',
          muted: 'rgba(52,211,153,0.10)',
        },
        warning: {
          DEFAULT: '#fbbf24',
          muted: 'rgba(251,191,36,0.10)',
        },
        error: {
          DEFAULT: '#f87171',
          muted: 'rgba(248,113,113,0.10)',
        },
        info: {
          DEFAULT: '#60a5fa',
          muted: 'rgba(96,165,250,0.10)',
        },
        premium: {
          DEFAULT: '#a78bfa',
          muted: 'rgba(167,139,250,0.10)',
        },
      },
      fontFamily: {
        sans: ['Inter', '-apple-system', 'BlinkMacSystemFont', 'sans-serif'],
        mono: ['JetBrains Mono', 'monospace'],
      },
    },
  },
}

```