#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use std::env;
use std::io::{BufRead, BufReader, Read, Write};
use std::net::TcpStream;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::sync::{Arc, Mutex, OnceLock};
use std::thread;
use std::time::{Duration, Instant};
use tauri::Manager;

#[derive(Debug, Clone, Serialize, Deserialize)]
struct ProcessInfo {
    pid: u32,
    name: String,
    #[serde(rename = "imagePath")]
    image_path: Option<String>,
    #[serde(rename = "cpuPercent")]
    cpu_percent: f64,
    #[serde(rename = "ramMb")]
    ram_mb: f64,
    status: String,
    priority: String,
    #[serde(rename = "isCritical")]
    is_critical: Option<bool>,
    #[serde(rename = "riskLabel")]
    risk_label: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct StartupItem {
    name: String,
    publisher: String,
    location: String,
    enabled: bool,
    impact: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct ServiceInfo {
    name: String,
    #[serde(rename = "displayName")]
    display_name: String,
    status: String,
    #[serde(rename = "startType")]
    start_type: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct DriverInfo {
    name: String,
    manufacturer: String,
    version: String,
    status: String,
    date: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct StorageCategory {
    name: String,
    #[serde(rename = "sizeGb")]
    size_gb: f64,
    percent: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct SystemSnapshot {
    #[serde(rename = "cpuPercent")]
    cpu_percent: f64,
    #[serde(rename = "ramPercent")]
    ram_percent: f64,
    #[serde(rename = "ramUsedMb")]
    ram_used_mb: f64,
    #[serde(rename = "ramTotalMb")]
    ram_total_mb: f64,
    #[serde(rename = "diskPercent")]
    disk_percent: f64,
    #[serde(rename = "diskUsedGb")]
    disk_used_gb: f64,
    #[serde(rename = "diskTotalGb")]
    disk_total_gb: f64,
    #[serde(rename = "netDownloadKbps")]
    net_download_kbps: f64,
    #[serde(rename = "netUploadKbps")]
    net_upload_kbps: f64,
    #[serde(rename = "gpuPercent")]
    gpu_percent: f64,
    #[serde(rename = "temperatureCelsius")]
    temperature_celsius: f64,
    #[serde(rename = "fanRpm")]
    fan_rpm: f64,
    #[serde(rename = "healthScore")]
    health_score: f64,
    #[serde(rename = "healthSummary")]
    health_summary: String,
    issues: Vec<String>,
    processes: Vec<ProcessInfo>,
    #[serde(rename = "startupItems")]
    startup_items: Vec<StartupItem>,
    services: Vec<ServiceInfo>,
    drivers: Vec<DriverInfo>,
    #[serde(rename = "storageCategories")]
    storage_categories: Vec<StorageCategory>,
    timestamp: i64,
}

#[derive(Clone, Default)]
struct AppState {
    latest_snapshot: Arc<Mutex<Option<SystemSnapshot>>>,
    last_error: Arc<Mutex<Option<String>>>,
    advisor_summary_cache: Arc<Mutex<Option<CachedValue>>>,
    home_summary_cache: Arc<Mutex<Option<CachedValue>>>,
    backup_summary_cache: Arc<Mutex<Option<CachedValue>>>,
}

#[derive(Clone)]
struct CachedValue {
    value: Value,
    captured_at: Instant,
}

static BACKEND_DAEMON_BOOT_LOCK: OnceLock<Mutex<()>> = OnceLock::new();

fn backend_daemon_boot_lock() -> &'static Mutex<()> {
    BACKEND_DAEMON_BOOT_LOCK.get_or_init(|| Mutex::new(()))
}

fn daemon_addr() -> &'static str {
    "127.0.0.1:47321"
}

fn resolve_backend_exe() -> Result<PathBuf, String> {
    if let Ok(path) = env::var("PULSEBOOST_BACKEND_EXE") {
        let p = PathBuf::from(path);
        if p.exists() {
            return Ok(p);
        }
    }

    let cwd = env::current_dir().map_err(|e| format!("cwd error: {e}"))?;
    let exe_dir = env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(Path::to_path_buf))
        .unwrap_or_else(|| cwd.clone());

    let candidates = [
        cwd.join("../build_run/Release/PulseBoostAI.exe"),
        cwd.join("build_run/Release/PulseBoostAI.exe"),
        cwd.join("../../build_run/Release/PulseBoostAI.exe"),
        exe_dir.join("../../../../build_run/Release/PulseBoostAI.exe"),
        exe_dir.join("../PulseBoostAI.exe"),
    ];

    for candidate in candidates {
        if candidate.exists() {
            return Ok(candidate);
        }
    }

    Err("Unable to locate PulseBoostAI.exe. Set PULSEBOOST_BACKEND_EXE env var.".to_string())
}

fn run_backend_cli(args: &[String]) -> Result<String, String> {
    let exe = resolve_backend_exe()?;
    let mut child = Command::new(exe)
        .args(args)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|e| format!("Backend execute failed: {e}"))?;

    let timeout = Duration::from_secs(20);
    let start = Instant::now();
    loop {
        match child
            .try_wait()
            .map_err(|e| format!("Backend wait failed: {e}"))?
        {
            Some(_) => break,
            None => {
                if start.elapsed() >= timeout {
                    let _ = child.kill();
                    let _ = child.wait();
                    return Err("Backend command timed out after 20s".to_string());
                }
                thread::sleep(Duration::from_millis(30));
            }
        }
    }

    let output = child
        .wait_with_output()
        .map_err(|e| format!("Backend output read failed: {e}"))?;

    if !output.status.success() {
        let stderr = String::from_utf8_lossy(&output.stderr).trim().to_string();
        let stdout = String::from_utf8_lossy(&output.stdout).trim().to_string();
        let reason = if !stderr.is_empty() { stderr } else { stdout };
        return Err(if reason.is_empty() {
            format!("Backend command failed with code {:?}", output.status.code())
        } else {
            reason
        });
    }

    Ok(String::from_utf8_lossy(&output.stdout).trim().to_string())
}

fn daemon_ping() -> bool {
    let Ok(mut stream) = TcpStream::connect(daemon_addr()) else {
        return false;
    };
    let request = format!("{}\n", json!({"id": "ping", "argv": ["--ping"]}));
    if stream.write_all(request.as_bytes()).is_err() {
        return false;
    }
    let mut line = String::new();
    let mut reader = BufReader::new(stream);
    if reader.read_line(&mut line).is_err() {
        return false;
    }
    serde_json::from_str::<Value>(&line)
        .ok()
        .and_then(|value| value.get("ok").and_then(Value::as_bool))
        .unwrap_or(false)
}

fn spawn_backend_daemon() -> Result<(), String> {
    let exe = resolve_backend_exe()?;
    Command::new(exe)
        .arg("--daemon")
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
        .map_err(|e| format!("Daemon start failed: {e}"))?;
    Ok(())
}

fn ensure_backend_daemon() -> Result<(), String> {
    if daemon_ping() {
        return Ok(());
    }

    let _guard = backend_daemon_boot_lock()
        .lock()
        .map_err(|_| "Backend daemon lock poisoned".to_string())?;

    if daemon_ping() {
        return Ok(());
    }

    spawn_backend_daemon()?;
    let start = Instant::now();
    while start.elapsed() < Duration::from_secs(8) {
        if daemon_ping() {
            return Ok(());
        }
        thread::sleep(Duration::from_millis(150));
    }

    Err("Backend daemon failed to start".to_string())
}

fn daemon_request(args: &[String]) -> Result<String, String> {
    ensure_backend_daemon()?;
    let mut stream = TcpStream::connect(daemon_addr())
        .map_err(|e| format!("Daemon connect failed: {e}"))?;
    let request = format!("{}\n", json!({"id": "req", "argv": args}));
    stream
        .write_all(request.as_bytes())
        .map_err(|e| format!("Daemon write failed: {e}"))?;

    let mut reader = BufReader::new(stream);
    let mut line = String::new();
    reader
        .read_line(&mut line)
        .map_err(|e| format!("Daemon read failed: {e}"))?;
    let value = serde_json::from_str::<Value>(line.trim())
        .map_err(|e| format!("Daemon response parse error: {e}"))?;

    if !value.get("supported").and_then(Value::as_bool).unwrap_or(true) {
        return Err("unsupported-daemon-command".to_string());
    }

    let stdout = value
        .get("stdout")
        .and_then(Value::as_str)
        .unwrap_or_default()
        .to_string();
    let exit_code = value.get("exitCode").and_then(Value::as_i64).unwrap_or(0);
    if exit_code != 0 {
        return Err(stdout);
    }
    Ok(stdout)
}

fn run_backend(args: &[String]) -> Result<String, String> {
    match daemon_request(args) {
        Ok(output) => Ok(output),
        Err(error) if error == "unsupported-daemon-command" => run_backend_cli(args),
        Err(_) => run_backend_cli(args),
    }
}

fn parse_ok_response(payload: &str) -> bool {
    if payload.is_empty() {
        return true;
    }
    if let Ok(value) = serde_json::from_str::<Value>(payload) {
        return value.get("ok").and_then(Value::as_bool).unwrap_or(true);
    }
    true
}

fn parse_backend_value(output: &str) -> Value {
    serde_json::from_str::<Value>(output).unwrap_or_else(|_| json!({ "ok": parse_ok_response(output), "raw": output }))
}

fn parse_backend_json(args: &[String], label: &str) -> Result<Value, String> {
    let output = run_backend(args)?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("{label} parse error: {e}"))
}

fn load_cached_value(cache: &Mutex<Option<CachedValue>>, ttl: Duration) -> Option<Value> {
    let guard = cache.lock().ok()?;
    let cached = guard.as_ref()?;
    if cached.captured_at.elapsed() <= ttl {
        return Some(cached.value.clone());
    }
    None
}

fn store_cached_value(cache: &Mutex<Option<CachedValue>>, value: &Value) {
    if let Ok(mut guard) = cache.lock() {
        *guard = Some(CachedValue {
            value: value.clone(),
            captured_at: Instant::now(),
        });
    }
}

fn invalidate_summary_caches(state: &AppState) {
    for cache in [
        &state.advisor_summary_cache,
        &state.home_summary_cache,
        &state.backup_summary_cache,
    ] {
        if let Ok(mut guard) = cache.lock() {
            *guard = None;
        }
    }
}

fn extract_error_message(value: &Value, fallback_action: &str) -> String {
    if let Some(reason) = value.get("reason").and_then(Value::as_str) {
        return reason.to_string();
    }
    if let Some(error) = value.get("error").and_then(Value::as_str) {
        return error.to_string();
    }
    format!("{fallback_action} failed")
}

fn compute_pulse_score(snapshot: &SystemSnapshot, tweaks: &[Value], benchmark_history: &[Value]) -> Value {
    let tweaks_applied = tweaks
        .iter()
        .filter(|item| item.get("isApplicable").and_then(Value::as_bool).unwrap_or(false))
        .filter(|item| item.get("isApplied").and_then(Value::as_bool).unwrap_or(false))
        .count() as i64;
    let tweaks_available = tweaks
        .iter()
        .filter(|item| item.get("isApplicable").and_then(Value::as_bool).unwrap_or(false))
        .count() as i64;

    let ram_gb = snapshot.ram_total_mb / 1024.0;
    let mut hardware_tier = ram_gb.clamp(8.75, 35.0) * 4.0;
    hardware_tier += if snapshot.disk_total_gb >= 1024.0 {
        45.0
    } else if snapshot.disk_total_gb >= 512.0 {
        30.0
    } else {
        15.0
    };
    hardware_tier += if snapshot.gpu_percent > 0.0 { 70.0 } else { 30.0 };
    hardware_tier += if snapshot.health_score >= 80.0 {
        35.0
    } else if snapshot.health_score >= 60.0 {
        25.0
    } else {
        15.0
    };
    hardware_tier = hardware_tier.clamp(0.0, 300.0);

    let optimization_level = if tweaks_available > 0 {
        ((tweaks_applied as f64 / tweaks_available as f64) * 400.0).round()
    } else {
        0.0
    };
    let health_state = (snapshot.health_score * 2.0).round().clamp(0.0, 200.0);

    let latest_benchmark = benchmark_history.last().cloned();
    let bench_score = latest_benchmark
        .as_ref()
        .and_then(|value| value.get("pulseScore"))
        .and_then(Value::as_f64)
        .map(|score| (score / 10.0).round().clamp(0.0, 100.0))
        .unwrap_or(50.0);

    let total = (hardware_tier + optimization_level + health_state + bench_score)
        .round()
        .clamp(0.0, 1000.0) as i64;
    let percentile = (((total as f64 - 250.0) / 7.5).round() as i64).clamp(1, 99);
    let grade = if total >= 900 {
        "S"
    } else if total >= 750 {
        "A"
    } else if total >= 600 {
        "B"
    } else if total >= 450 {
        "C"
    } else if total >= 300 {
        "D"
    } else {
        "F"
    };

    json!({
        "total": total,
        "hardwareTier": hardware_tier.round() as i64,
        "optimizationLevel": optimization_level.round() as i64,
        "healthState": health_state.round() as i64,
        "benchScore": bench_score.round() as i64,
        "grade": grade,
        "percentile": percentile,
        "tweaksApplied": tweaks_applied,
        "tweaksAvailable": tweaks_available
    })
}

fn get_benchmark_history_value() -> Result<Value, String> {
    parse_backend_json(&["--benchmark-history".to_string()], "Benchmark history")
}

fn get_recent_actions_value() -> Result<Value, String> {
    parse_backend_json(&["--recent-actions-json".to_string()], "Recent actions")
}

fn get_snapshots_value() -> Result<Value, String> {
    parse_backend_json(&["--list-snapshots".to_string()], "Snapshots")
}

fn get_system_advisor_summary_value(state: &AppState) -> Result<Value, String> {
    if let Some(value) = load_cached_value(&state.advisor_summary_cache, Duration::from_secs(45)) {
        return Ok(value);
    }
    let value = parse_backend_json(&["--system-advisor".to_string()], "System advisor")?;
    store_cached_value(&state.advisor_summary_cache, &value);
    Ok(value)
}

fn placeholder_snapshot() -> SystemSnapshot {
    SystemSnapshot {
        cpu_percent: 0.0,
        ram_percent: 0.0,
        ram_used_mb: 0.0,
        ram_total_mb: 1.0,
        disk_percent: 0.0,
        disk_used_gb: 0.0,
        disk_total_gb: 1.0,
        net_download_kbps: 0.0,
        net_upload_kbps: 0.0,
        gpu_percent: 0.0,
        temperature_celsius: 0.0,
        fan_rpm: 0.0,
        health_score: 0.0,
        health_summary: "Telemetry warming up".to_string(),
        issues: vec!["Collecting live system telemetry".to_string()],
        processes: Vec::new(),
        startup_items: Vec::new(),
        services: Vec::new(),
        drivers: Vec::new(),
        storage_categories: Vec::new(),
        timestamp: 0,
    }
}

fn get_snapshot_internal() -> Result<SystemSnapshot, String> {
    let output = run_backend(&["--snapshot-json".to_string()])?;
    serde_json::from_str::<SystemSnapshot>(&output)
        .map_err(|e| format!("Snapshot parse error: {e} | raw: {output}"))
}

fn emit_action_progress(window: &tauri::Window, action: &str, percent: u8, message: impl Into<String>) {
    let _ = window.emit(
        "action_progress",
        json!({
            "action": action,
            "percent": percent,
            "message": message.into()
        }),
    );
}

fn emit_action_complete(window: &tauri::Window, action: &str, success: bool, result: Value) {
    let _ = window.emit(
        "action_complete",
        json!({
            "action": action,
            "success": success,
            "result": result
        }),
    );
}

fn run_progress_action(
    window: &tauri::Window,
    action: &str,
    args: &[String],
    start_message: &str,
    running_message: &str,
    done_message: &str,
) -> Result<Value, String> {
    emit_action_progress(window, action, 5, start_message);
    emit_action_progress(window, action, 45, running_message);

    match run_backend(args) {
        Ok(output) => {
            let value = parse_backend_value(&output);
            let success = value.get("ok").and_then(Value::as_bool).unwrap_or(true);
            emit_action_progress(window, action, 100, done_message);
            emit_action_complete(window, action, success, value.clone());
            if success {
                Ok(value)
            } else {
                Err(extract_error_message(&value, action))
            }
        }
        Err(error) => {
            let result = json!({ "ok": false, "error": error.clone() });
            emit_action_complete(window, action, false, result);
            Err(error)
        }
    }
}

fn default_user_scan_paths() -> Vec<String> {
    let mut paths = Vec::new();
    if let Ok(profile) = env::var("USERPROFILE") {
        for folder in ["Desktop", "Documents", "Downloads", "Pictures", "Videos", "Music"] {
            let candidate = PathBuf::from(&profile).join(folder);
            if candidate.exists() {
                paths.push(candidate.to_string_lossy().to_string());
            }
        }
    }
    paths
}

fn choose_startup_candidate(snapshot: &SystemSnapshot) -> Option<String> {
    snapshot
        .startup_items
        .iter()
        .filter(|item| item.enabled)
        .max_by_key(|item| match item.impact.as_str() {
            "high" => 3,
            "medium" => 2,
            "low" => 1,
            _ => 0,
        })
        .map(|item| item.name.clone())
}

fn run_security_remediation(window: &tauri::Window, issue: Option<String>) -> Result<Value, String> {
    let snapshot = get_snapshot_internal()?;
    let requested_issue = issue.unwrap_or_default();
    let lower_issue = requested_issue.to_lowercase();
    let mut actions: Vec<Value> = Vec::new();

    emit_action_progress(window, "security_remediation", 5, "Preparing remediation");

    if lower_issue.contains("driver") {
        let result = json!({
            "ok": false,
            "reason": "manual-driver-review-required",
            "message": "Driver issues require manual review. Automated driver replacement is disabled in GA shell."
        });
        emit_action_complete(window, "security_remediation", false, result.clone());
        return Err("manual-driver-review-required".to_string());
    }

    if lower_issue.contains("startup") || snapshot.startup_items.iter().filter(|item| item.enabled).count() > 20 {
        if let Some(name) = choose_startup_candidate(&snapshot) {
            emit_action_progress(window, "security_remediation", 35, format!("Disabling startup item: {name}"));
            let output = run_backend(&["--disable-startup-by-name".to_string(), name.clone()])?;
            let value = parse_backend_value(&output);
            if value.get("ok").and_then(Value::as_bool).unwrap_or(false) {
                actions.push(json!({ "action": "disable_startup_item", "target": name, "result": value }));
            }
        }
    }

    if lower_issue.is_empty() || lower_issue.contains("risk") || lower_issue.contains("threat") || lower_issue.contains("critical") {
        emit_action_progress(window, "security_remediation", 65, "Applying low-risk network hardening");
        let flush = parse_backend_value(&run_backend(&["--flush-dns".to_string()])?);
        actions.push(json!({ "action": "flush_dns", "result": flush }));
        let tcp = parse_backend_value(&run_backend(&["--optimize-tcp".to_string()])?);
        actions.push(json!({ "action": "optimize_tcp", "result": tcp }));
    }

    if actions.is_empty() {
        let result = json!({
            "ok": true,
            "message": if requested_issue.is_empty() {
                "No automated low-risk remediation was required."
            } else {
                "No direct automated remediation was required for this finding."
            },
            "actions": []
        });
        emit_action_progress(window, "security_remediation", 100, "No remediation needed");
        emit_action_complete(window, "security_remediation", true, result.clone());
        return Ok(result);
    }

    let result = json!({
        "ok": true,
        "actions": actions
    });
    emit_action_progress(window, "security_remediation", 100, "Remediation complete");
    emit_action_complete(window, "security_remediation", true, result.clone());
    Ok(result)
}

#[tauri::command]
fn get_pulse_score(state: tauri::State<AppState>) -> Result<Value, String> {
    let home = get_home_summary(state)?;
    Ok(home
        .get("pulseScore")
        .cloned()
        .unwrap_or_else(|| json!({ "total": 0, "grade": "F", "percentile": 1 })))
}
#[tauri::command]
fn list_tweaks() -> Result<Value, String> {
    let output = run_backend(&["--list-tweaks".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Tweak payload parse error: {e}"))
}

#[tauri::command]
fn apply_tweak(id: String) -> Result<Value, String> {
    let output = run_backend(&["--apply-tweak".to_string(), id])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Apply tweak payload parse error: {e}"))
}

#[tauri::command]
fn revert_tweak(id: String) -> Result<Value, String> {
    let output = run_backend(&["--revert-tweak".to_string(), id])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Revert tweak payload parse error: {e}"))
}

#[tauri::command]
fn apply_safe_tweaks() -> Result<Value, String> {
    let output = run_backend(&["--apply-safe-tweaks".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Apply safe tweaks parse error: {e}"))
}

#[tauri::command]
fn apply_high_impact_tweaks() -> Result<Value, String> {
    let output = run_backend(&["--apply-high-impact-tweaks".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Apply high impact tweaks parse error: {e}"))
}

#[tauri::command]
fn revert_all_tweaks() -> Result<Value, String> {
    let output = run_backend(&["--revert-all-tweaks".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Revert all tweaks parse error: {e}"))
}

#[tauri::command]
fn quick_benchmark(window: tauri::Window) -> Result<Value, String> {
    run_progress_action(
        &window,
        "quick_benchmark",
        &["--quick-benchmark".to_string()],
        "Preparing quick benchmark",
        "Running CPU, memory, disk, and network tests",
        "Quick benchmark complete",
    )
}

#[tauri::command]
fn full_benchmark(window: tauri::Window) -> Result<Value, String> {
    run_progress_action(
        &window,
        "full_benchmark",
        &["--full-benchmark".to_string()],
        "Preparing full benchmark",
        "Running extended benchmark suite",
        "Full benchmark complete",
    )
}

#[tauri::command]
fn get_benchmark_history() -> Result<Value, String> {
    let output = run_backend(&["--benchmark-history".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Benchmark history parse error: {e}"))
}

#[tauri::command]
fn get_system_advisor(state: tauri::State<AppState>) -> Result<Value, String> {
    get_system_advisor_summary_value(state.inner())
}

#[tauri::command]
fn get_system_advisor_summary(state: tauri::State<AppState>) -> Result<Value, String> {
    get_system_advisor_summary_value(state.inner())
}

#[tauri::command]
fn get_system_advisor_detail(state: tauri::State<AppState>) -> Result<Value, String> {
    get_system_advisor_summary_value(state.inner())
}

#[tauri::command]
fn detect_games() -> Result<Value, String> {
    let output = run_backend(&["--detect-games".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Game detection parse error: {e}"))
}

#[tauri::command]
fn get_home_summary(state: tauri::State<AppState>) -> Result<Value, String> {
    if let Some(value) = load_cached_value(&state.home_summary_cache, Duration::from_secs(20)) {
        return Ok(value);
    }

    let snapshot = if let Ok(guard) = state.latest_snapshot.lock() {
        guard.clone().unwrap_or_else(placeholder_snapshot)
    } else {
        placeholder_snapshot()
    };
    let tweaks = parse_backend_json(&["--list-tweaks".to_string()], "Tweak payload")?;
    let benchmark = get_benchmark_history_value()?;
    let actions = get_recent_actions_value()?;
    let advisor = load_cached_value(&state.advisor_summary_cache, Duration::from_secs(45))
        .unwrap_or_else(|| json!({ "ok": true, "items": [] }));

    let tweak_items = tweaks
        .get("tweaks")
        .and_then(Value::as_array)
        .cloned()
        .unwrap_or_default();
    let benchmark_history = benchmark
        .get("history")
        .and_then(Value::as_array)
        .cloned()
        .unwrap_or_default();
    let pulse_score = compute_pulse_score(&snapshot, &tweak_items, &benchmark_history);

    let latest_benchmark = benchmark_history.last().cloned();
    let latest_delta = benchmark.get("latestDelta").cloned();

    let value = json!({
        "ok": true,
        "pulseScore": pulse_score,
        "healthScore": snapshot.health_score,
        "healthSummary": snapshot.health_summary,
        "recentActions": actions.get("actions").cloned().unwrap_or_else(|| json!([])),
        "advisorItems": advisor.get("items").cloned().unwrap_or_else(|| json!([])),
        "benchmarkSummary": {
            "runs": benchmark_history.len(),
            "latest": latest_benchmark,
            "latestDelta": latest_delta
        }
    });
    store_cached_value(&state.home_summary_cache, &value);
    Ok(value)
}

#[tauri::command]
fn get_backup_summary(state: tauri::State<AppState>) -> Result<Value, String> {
    if let Some(value) = load_cached_value(&state.backup_summary_cache, Duration::from_secs(15)) {
        return Ok(value);
    }

    let actions = get_recent_actions_value()?;
    let snapshots = get_snapshots_value()?;
    let value = json!({
        "ok": true,
        "actions": actions.get("actions").cloned().unwrap_or_else(|| json!([])),
        "snapshots": snapshots.get("snapshots").cloned().unwrap_or_else(|| json!([])),
        "guidance": {
            "backupRecommended": true,
            "message": "Create a restore point or snapshot before applying large batches of changes."
        }
    });
    store_cached_value(&state.backup_summary_cache, &value);
    Ok(value)
}

#[tauri::command]
fn get_optimization_presets() -> Result<Value, String> {
    Ok(json!({
        "ok": true,
        "presets": [
            {
                "id": "safe_recommended",
                "label": "Safe Recommended",
                "description": "Low-risk baseline optimizations with reversibility.",
                "impact": "high",
                "recommended": true,
                "proOnly": false,
                "action": "apply_safe_tweaks"
            },
            {
                "id": "fps_latency",
                "label": "FPS and Latency",
                "description": "Focuses on frame delivery, mouse response, and network responsiveness.",
                "impact": "high",
                "recommended": true,
                "proOnly": false,
                "tweakIds": [
                    "power_disable_power_throttling",
                    "gaming_disable_mouse_acceleration",
                    "gaming_enable_game_mode",
                    "network_disable_network_throttling",
                    "network_system_responsiveness"
                ]
            },
            {
                "id": "network_ping",
                "label": "Network and Ping",
                "description": "Targets gaming network queueing and QoS reservation overhead.",
                "impact": "high",
                "recommended": false,
                "proOnly": false,
                "tweakIds": [
                    "network_disable_network_throttling",
                    "network_system_responsiveness",
                    "network_remove_reserved_bandwidth"
                ]
            },
            {
                "id": "quality_of_life",
                "label": "Quality of Life",
                "description": "Reduces Windows shell overhead and visual distractions.",
                "impact": "medium",
                "recommended": false,
                "proOnly": false,
                "tweakIds": [
                    "windows_disable_transparency",
                    "windows_disable_animations",
                    "windows_disable_aero_peek",
                    "windows_disable_taskbar_animations"
                ]
            },
            {
                "id": "privacy",
                "label": "Privacy",
                "description": "Disables telemetry, activity publishing, and feedback prompts.",
                "impact": "medium",
                "recommended": false,
                "proOnly": false,
                "tweakIds": [
                    "privacy_disable_telemetry",
                    "privacy_disable_activity_history",
                    "privacy_disable_feedback_notifications",
                    "privacy_disable_advertising_id"
                ]
            },
            {
                "id": "advanced",
                "label": "Advanced",
                "description": "Higher-impact changes for dedicated performance systems.",
                "impact": "high",
                "recommended": false,
                "proOnly": true,
                "tweakIds": [
                    "gpu_enable_hardware_gpu_scheduling",
                    "windows_disable_sysmain",
                    "windows_disable_search"
                ]
            }
        ]
    }))
}

#[tauri::command]
fn apply_optimization_preset(window: tauri::Window, state: tauri::State<AppState>, preset_id: String) -> Result<Value, String> {
    let preset = preset_id.trim().to_lowercase();
    emit_action_progress(&window, "optimization_preset", 5, format!("Preparing preset: {preset}"));

    let result = if preset == "safe_recommended" {
        parse_backend_json(&["--apply-safe-tweaks".to_string()], "Apply safe tweaks")?
    } else {
        let preset_map: Vec<&str> = match preset.as_str() {
            "fps_latency" => vec![
                "power_disable_power_throttling",
                "gaming_disable_mouse_acceleration",
                "gaming_enable_game_mode",
                "network_disable_network_throttling",
                "network_system_responsiveness",
            ],
            "network_ping" => vec![
                "network_disable_network_throttling",
                "network_system_responsiveness",
                "network_remove_reserved_bandwidth",
            ],
            "quality_of_life" => vec![
                "windows_disable_transparency",
                "windows_disable_animations",
                "windows_disable_aero_peek",
                "windows_disable_taskbar_animations",
            ],
            "privacy" => vec![
                "privacy_disable_telemetry",
                "privacy_disable_activity_history",
                "privacy_disable_feedback_notifications",
                "privacy_disable_advertising_id",
            ],
            "advanced" => vec![
                "gpu_enable_hardware_gpu_scheduling",
                "windows_disable_sysmain",
                "windows_disable_search",
            ],
            _ => return Err("Unknown optimization preset".to_string()),
        };

        let mut results = Vec::new();
        for (index, tweak_id) in preset_map.iter().enumerate() {
            let percent = 15 + (((index + 1) as f32 / preset_map.len() as f32) * 75.0).round() as u8;
            emit_action_progress(&window, "optimization_preset", percent, format!("Applying {tweak_id}"));
            let value = parse_backend_json(
                &["--apply-tweak".to_string(), (*tweak_id).to_string()],
                "Apply tweak",
            )?;
            results.push(value);
        }

        json!({
            "ok": true,
            "presetId": preset,
            "results": results
        })
    };

    invalidate_summary_caches(state.inner());
    emit_action_progress(&window, "optimization_preset", 100, "Preset applied");
    emit_action_complete(&window, "optimization_preset", true, result.clone());
    Ok(result)
}

#[tauri::command]
fn get_boost_up_actions() -> Result<Value, String> {
    Ok(json!({
        "ok": true,
        "actions": [
            { "id": "pre_game_clean", "label": "Pre-Game Clean", "description": "Flush temporary clutter and memory pressure before a session.", "duration": "~15s", "tone": "accent" },
            { "id": "clean_junk", "label": "Clean Junk", "description": "Remove disposable files and reclaim storage.", "duration": "~3s", "tone": "info" },
            { "id": "optimize_ram", "label": "Optimize RAM", "description": "Trim working sets and recover active memory.", "duration": "~2s", "tone": "info" },
            { "id": "flush_standby_list", "label": "Flush Standby List", "description": "Force-recover reclaimable cached memory.", "duration": "~2s", "tone": "warn" },
            { "id": "optimize_disk", "label": "Optimize Drive", "description": "Run the system drive optimization path.", "duration": "~10s", "tone": "warn" },
            { "id": "network_reset", "label": "Refresh Network", "description": "Flush DNS and re-apply the TCP optimization path.", "duration": "~4s", "tone": "info" }
        ]
    }))
}

#[tauri::command]
fn run_boost_up_action(window: tauri::Window, state: tauri::State<AppState>, action_id: String) -> Result<Value, String> {
    let action = action_id.trim().to_lowercase();
    emit_action_progress(&window, "boost_up", 5, format!("Preparing {action}"));

    let result = match action.as_str() {
        "pre_game_clean" => {
            let steps = [
                ("--clean", "Cleaning disposable files"),
                ("--optimize-ram", "Optimizing RAM"),
                ("--flush-standby", "Flushing standby list"),
                ("--flush-dns", "Refreshing DNS cache"),
            ];
            let mut results = Vec::new();
            for (index, (flag, message)) in steps.iter().enumerate() {
                let percent = 15 + (((index + 1) as f32 / steps.len() as f32) * 75.0).round() as u8;
                emit_action_progress(&window, "boost_up", percent, *message);
                results.push(parse_backend_json(&[flag.to_string()], "Boost-up step")?);
            }
            json!({ "ok": true, "action": action, "results": results })
        }
        "clean_junk" => run_progress_action(
            &window,
            "boost_up",
            &["--clean".to_string()],
            "Preparing cleanup",
            "Cleaning disposable files",
            "Cleanup complete",
        )?,
        "optimize_ram" => parse_backend_json(&["--optimize-ram".to_string()], "Optimize RAM")?,
        "flush_standby_list" => parse_backend_json(&["--flush-standby".to_string()], "Flush standby")?,
        "optimize_disk" => run_progress_action(
            &window,
            "boost_up",
            &["--optimize-disk".to_string()],
            "Preparing drive optimization",
            "Optimizing the system drive",
            "Drive optimization complete",
        )?,
        "network_reset" => {
            let flush = parse_backend_json(&["--flush-dns".to_string()], "Flush DNS")?;
            emit_action_progress(&window, "boost_up", 65, "Applying TCP optimization");
            let tcp = parse_backend_json(&["--optimize-tcp".to_string()], "Optimize TCP")?;
            json!({ "ok": true, "action": action, "results": [flush, tcp] })
        }
        _ => return Err("Unknown Boost-Up action".to_string()),
    };

    invalidate_summary_caches(state.inner());
    emit_action_progress(&window, "boost_up", 100, "Boost-Up action complete");
    emit_action_complete(&window, "boost_up", true, result.clone());
    Ok(result)
}
#[tauri::command]
fn get_snapshot(state: tauri::State<AppState>) -> Result<SystemSnapshot, String> {
    if let Ok(guard) = state.latest_snapshot.lock() {
        if let Some(snapshot) = &*guard {
            return Ok(snapshot.clone());
        }
    }
    Ok(placeholder_snapshot())
}

#[tauri::command]
fn ask_agent(window: tauri::Window, message: String) -> Result<String, String> {
    if let Ok(()) = ensure_backend_daemon() {
        let mut stream = TcpStream::connect(daemon_addr())
            .map_err(|e| format!("Daemon connect failed: {e}"))?;
        let request = format!(
            "{}
",
            json!({ "id": "chat", "argv": ["--chat-stream", message.clone()] })
        );
        stream
            .write_all(request.as_bytes())
            .map_err(|e| format!("Daemon write failed: {e}"))?;

        let mut final_reply = String::new();
        let reader = BufReader::new(stream);
        for line_result in reader.lines() {
            let line = line_result.map_err(|e| format!("Chat stream read failed: {e}"))?;
            if line.trim().is_empty() {
                continue;
            }
            if let Ok(value) = serde_json::from_str::<Value>(&line) {
                match value.get("type").and_then(Value::as_str).unwrap_or_default() {
                    "chunk" => {
                        if let Some(chunk) = value.get("chunk").and_then(Value::as_str) {
                            let _ = window.emit("agent_stream_chunk", chunk.to_string());
                        }
                    }
                    "done" => {
                        final_reply = value
                            .get("reply")
                            .and_then(Value::as_str)
                            .unwrap_or_default()
                            .to_string();
                    }
                    _ => {}
                }
            }
        }
        return Ok(final_reply);
    }

    let exe = resolve_backend_exe()?;
    let mut child = Command::new(exe)
        .args(["--chat-stream", &message])
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|e| format!("Backend execute failed: {e}"))?;

    let stdout = child
        .stdout
        .take()
        .ok_or_else(|| "Failed to capture backend stdout".to_string())?;
    let mut stderr = child
        .stderr
        .take()
        .ok_or_else(|| "Failed to capture backend stderr".to_string())?;

    let mut final_reply = String::new();
    let reader = BufReader::new(stdout);
    for line_result in reader.lines() {
        let line = line_result.map_err(|e| format!("Chat stream read failed: {e}"))?;
        if line.trim().is_empty() {
            continue;
        }

        if let Ok(value) = serde_json::from_str::<Value>(&line) {
            match value.get("type").and_then(Value::as_str).unwrap_or_default() {
                "chunk" => {
                    if let Some(chunk) = value.get("chunk").and_then(Value::as_str) {
                        let _ = window.emit("agent_stream_chunk", chunk.to_string());
                    }
                }
                "done" => {
                    final_reply = value
                        .get("reply")
                        .and_then(Value::as_str)
                        .unwrap_or_default()
                        .to_string();
                }
                _ => {}
            }
        }
    }

    let mut stderr_text = String::new();
    let _ = stderr.read_to_string(&mut stderr_text);
    let status = child
        .wait()
        .map_err(|e| format!("Backend wait failed: {e}"))?;

    if !status.success() {
        let reason = stderr_text.trim();
        return Err(if reason.is_empty() {
            format!("Backend chat failed with code {:?}", status.code())
        } else {
            reason.to_string()
        });
    }

    Ok(final_reply)
}

#[tauri::command]
fn refresh_all(state: tauri::State<AppState>) -> Result<bool, String> {
    let output = run_backend(&["--refresh-all".to_string()])?;
    let parsed = serde_json::from_str::<SystemSnapshot>(&output).ok();
    if let Some(snapshot) = parsed {
        if let Ok(mut guard) = state.latest_snapshot.lock() {
            *guard = Some(snapshot);
        }
        if let Ok(mut guard) = state.last_error.lock() {
            *guard = None;
        }
        return Ok(true);
    }
    Err("refresh_all returned non-snapshot payload".to_string())
}

#[tauri::command]
fn clean_junk(window: tauri::Window) -> Result<Value, String> {
    run_progress_action(
        &window,
        "clean_junk",
        &["--clean".to_string()],
        "Preparing junk cleanup",
        "Cleaning disposable files",
        "Cleanup complete",
    )
}

#[tauri::command]
fn optimize_ram() -> Result<bool, String> {
    let output = run_backend(&["--optimize-ram".to_string()])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn optimize_disk(window: tauri::Window) -> Result<Value, String> {
    run_progress_action(
        &window,
        "optimize_disk",
        &["--optimize-disk".to_string()],
        "Preparing drive optimization",
        "Optimizing system drive",
        "Drive optimization complete",
    )
}

#[tauri::command]
fn flush_dns() -> Result<bool, String> {
    let output = run_backend(&["--flush-dns".to_string()])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn optimize_tcp() -> Result<bool, String> {
    let output = run_backend(&["--optimize-tcp".to_string()])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn refresh_latency() -> Result<i32, String> {
    let output = run_backend(&["--check-latency".to_string()])?;
    let value = serde_json::from_str::<Value>(&output)
        .ok()
        .and_then(|v| v.get("latency").and_then(Value::as_i64))
        .unwrap_or(-1);
    Ok(value as i32)
}

#[tauri::command]
fn get_network_diagnostics() -> Result<Value, String> {
    let output = run_backend(&["--network-diagnostics".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Network diagnostics parse error: {e}"))
}

#[tauri::command]
fn get_ram_breakdown() -> Result<Value, String> {
    let output = run_backend(&["--ram-breakdown".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("RAM breakdown parse error: {e}"))
}

#[tauri::command]
fn open_file_location(path: String) -> Result<bool, String> {
    let output = run_backend(&["--open-file-location".to_string(), path])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn delete_large_file(path: String) -> Result<Value, String> {
    let output = run_backend(&["--delete-file".to_string(), path])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Delete file parse error: {e}"))
}

#[tauri::command]
fn kill_process(pid: i64) -> Result<bool, String> {
    let output = run_backend(&["--kill-pid".to_string(), pid.to_string()])?;
    let value = parse_backend_value(&output);
    let success = value.get("ok").and_then(Value::as_bool).unwrap_or(false);
    if success {
        return Ok(true);
    }

    let reason = value.get("reason").and_then(Value::as_str).unwrap_or("kill_process failed");
    let message = if reason == "critical-process" {
        "Refused to terminate critical system process".to_string()
    } else {
        extract_error_message(&value, "kill_process")
    };
    Err(message)
}

#[tauri::command]
fn suspend_process(pid: i64) -> Result<bool, String> {
    let output = run_backend(&["--suspend-pid".to_string(), pid.to_string()])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn resume_process(pid: i64) -> Result<bool, String> {
    let output = run_backend(&["--resume-pid".to_string(), pid.to_string()])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn create_restore_point() -> Result<bool, String> {
    let output = run_backend(&["--create-restore-point".to_string()])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn enable_game_mode() -> Result<bool, String> {
    let output = run_backend(&["--enable-game-mode".to_string()])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn disable_game_mode() -> Result<bool, String> {
    let output = run_backend(&["--disable-game-mode".to_string()])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn optimize_game(executable_name: String) -> Result<Value, String> {
    let output = run_backend(&["--optimize-game".to_string(), executable_name])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Optimize game parse error: {e}"))
}

#[tauri::command]
fn launch_optimized_game(executable_name: String) -> Result<Value, String> {
    let output = run_backend(&["--launch-optimized-game".to_string(), executable_name])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Launch optimized game parse error: {e}"))
}

#[tauri::command]
fn revert_game_optimization() -> Result<bool, String> {
    let output = run_backend(&["--revert-game-optimization".to_string()])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn full_scan(window: tauri::Window) -> Result<Value, String> {
    run_progress_action(
        &window,
        "full_scan",
        &["--scan".to_string()],
        "Preparing full scan",
        "Collecting telemetry and health data",
        "Full scan complete",
    )
}

#[tauri::command]
fn take_snapshot() -> Result<String, String> {
    let output = run_backend(&["--take-snapshot".to_string()])?;
    let path = serde_json::from_str::<Value>(&output)
        .ok()
        .and_then(|v| v.get("path").and_then(Value::as_str).map(str::to_string))
        .unwrap_or_default();
    Ok(path)
}

#[tauri::command]
fn run_security_scan(window: tauri::Window) -> Result<Value, String> {
    run_progress_action(
        &window,
        "run_security_scan",
        &["--run-security-scan".to_string()],
        "Preparing security scan",
        "Evaluating security posture",
        "Security scan complete",
    )
}

#[tauri::command]
fn autofix_low_risk(window: tauri::Window) -> Result<Value, String> {
    run_security_remediation(&window, None)
}

#[tauri::command]
fn fix_security_issue(window: tauri::Window, issue: String) -> Result<Value, String> {
    run_security_remediation(&window, Some(issue))
}

#[tauri::command]
fn find_duplicates(window: tauri::Window, paths: Option<Vec<String>>) -> Result<Value, String> {
    let mut args = vec!["--find-duplicates".to_string()];
    if let Some(paths) = paths {
        args.extend(paths);
    }
    run_progress_action(
        &window,
        "find_duplicates",
        &args,
        "Preparing duplicate scan",
        "Hashing files and grouping duplicates",
        "Duplicate scan complete",
    )
}

#[tauri::command]
fn toggle_startup_item(name: String, enabled: bool) -> Result<bool, String> {
    let command = if enabled {
        "--enable-startup-by-name"
    } else {
        "--disable-startup-by-name"
    };
    let output = run_backend(&[command.to_string(), name])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn delay_startup_item(name: String, seconds: i32) -> Result<bool, String> {
    let output = run_backend(&[
        "--delay-startup-by-name".to_string(),
        name,
        seconds.max(1).to_string(),
    ])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn disable_selected_startup(names: Vec<String>) -> Result<bool, String> {
    if names.is_empty() {
        return Ok(true);
    }
    for name in names {
        let output = run_backend(&["--disable-startup-by-name".to_string(), name])?;
        if !parse_ok_response(&output) {
            return Ok(false);
        }
    }
    Ok(true)
}

#[tauri::command]
fn delay_selected_startup(names: Vec<String>, seconds: i32) -> Result<bool, String> {
    if names.is_empty() {
        return Ok(true);
    }
    for name in names {
        let output = run_backend(&[
            "--delay-startup-by-name".to_string(),
            name,
            seconds.max(1).to_string(),
        ])?;
        if !parse_ok_response(&output) {
            return Ok(false);
        }
    }
    Ok(true)
}

#[tauri::command]
fn get_scheduled_tasks() -> Result<Value, String> {
    let output = run_backend(&["--get-scheduled-tasks".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Scheduled task payload parse error: {e}"))
}

#[tauri::command]
fn set_scheduled_task(
    task_id: String,
    enabled: bool,
    task_type: String,
    interval_hours: i32,
) -> Result<bool, String> {
    let output = run_backend(&[
        "--set-scheduled-task".to_string(),
        task_id,
        if enabled { "true" } else { "false" }.to_string(),
        task_type,
        interval_hours.max(1).to_string(),
    ])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn get_snapshots() -> Result<Value, String> {
    let output = run_backend(&["--list-snapshots".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Snapshot list parse error: {e}"))
}

#[tauri::command]
fn restore_snapshot(window: tauri::Window, snapshot_path: String) -> Result<Value, String> {
    run_progress_action(
        &window,
        "restore_snapshot",
        &["--restore-snapshot".to_string(), snapshot_path],
        "Preparing snapshot restore",
        "Restoring startup baseline from snapshot",
        "Snapshot restore complete",
    )
}

#[tauri::command]
fn export_health_report() -> Result<Value, String> {
    let output = run_backend(&["--export-health-report".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Health report parse error: {e}"))
}
#[tauri::command]
fn get_recent_actions() -> Result<Value, String> {
    let output = run_backend(&["--recent-actions-json".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Recent action payload parse error: {e}"))
}

#[tauri::command]
fn scan_large_files(window: tauri::Window, paths: Option<Vec<String>>) -> Result<Value, String> {
    let mut args = vec!["--scan-large-files".to_string()];
    if let Some(paths) = paths {
        args.extend(paths);
    } else {
        args.extend(default_user_scan_paths());
    }
    run_progress_action(
        &window,
        "scan_large_files",
        &args,
        "Preparing large-file scan",
        "Scanning storage for oversized files",
        "Large-file scan complete",
    )
}

#[tauri::command]
fn get_game_mode_status() -> Result<Value, String> {
    let output = run_backend(&["--game-mode-status".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Game mode status parse error: {e}"))
}

#[tauri::command]
fn estimate_junk() -> Result<Value, String> {
    let output = run_backend(&["--estimate-junk".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Junk estimate parse error: {e}"))
}

#[tauri::command]
fn get_license_info() -> Result<Value, String> {
    let output = run_backend(&["--license-info".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("License info parse error: {e}"))
}

#[tauri::command]
fn activate_license(key: String) -> Result<Value, String> {
    let output = run_backend(&["--activate-license".to_string(), key])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("License activation parse error: {e}"))
}

#[tauri::command]
fn get_ai_preferences() -> Result<Value, String> {
    let output = run_backend(&["--get-ai-preferences".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("AI preferences parse error: {e}"))
}

#[tauri::command]
fn set_ai_preferences(mode: String, api_key: Option<String>) -> Result<Value, String> {
    let mut args = vec!["--set-ai-preferences".to_string(), mode];
    if let Some(api_key) = api_key {
        args.push(api_key);
    }
    let output = run_backend(&args)?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Set AI preferences parse error: {e}"))
}

#[tauri::command]
fn check_for_updates() -> Result<Value, String> {
    let output = run_backend(&["--check-for-updates".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Update status parse error: {e}"))
}

#[tauri::command]
fn get_error_log() -> Result<Value, String> {
    let output = run_backend(&["--error-log".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Error log parse error: {e}"))
}

#[tauri::command]
fn export_error_log() -> Result<Value, String> {
    let output = run_backend(&["--export-error-log".to_string()])?;
    serde_json::from_str::<Value>(&output)
        .map_err(|e| format!("Error log export parse error: {e}"))
}

#[tauri::command]
fn flush_standby_list() -> Result<bool, String> {
    let output = run_backend(&["--flush-standby".to_string()])?;
    Ok(parse_ok_response(&output))
}

#[tauri::command]
fn enable_ram_saver() -> Result<bool, String> {
    let output = run_backend(&["--enable-ram-saver".to_string()])?;
    Ok(parse_ok_response(&output))
}

fn main() {
    tauri::Builder::default()
        .manage(AppState::default())
        .setup(|app| {
            let handle = app.handle();
            let app_state = app.state::<AppState>().inner().clone();
            thread::spawn(move || loop {
                if ensure_backend_daemon().is_err() {
                    thread::sleep(Duration::from_secs(2));
                    continue;
                }

                match TcpStream::connect(daemon_addr()) {
                    Ok(mut stream) => {
                        let request = format!("{}\n", json!({ "id": "snapshot-sub", "subscribe": "snapshot_ready" }));
                        if stream.write_all(request.as_bytes()).is_err() {
                            thread::sleep(Duration::from_secs(2));
                            continue;
                        }

                        let reader = BufReader::new(stream);
                        for line_result in reader.lines() {
                            let Ok(line) = line_result else {
                                break;
                            };
                            if line.trim().is_empty() {
                                continue;
                            }
                            let Ok(value) = serde_json::from_str::<Value>(&line) else {
                                continue;
                            };
                            if value.get("event").and_then(Value::as_str) != Some("snapshot_ready") {
                                continue;
                            }
                            let Ok(snapshot) = serde_json::from_value::<SystemSnapshot>(value.get("payload").cloned().unwrap_or(Value::Null)) else {
                                continue;
                            };
                            if let Ok(mut guard) = app_state.latest_snapshot.lock() {
                                *guard = Some(snapshot.clone());
                            }
                            if let Ok(mut guard) = app_state.last_error.lock() {
                                *guard = None;
                            }
                            let _ = handle.emit_all("snapshot_ready", snapshot);
                        }
                    }
                    Err(error) => {
                        if let Ok(mut guard) = app_state.last_error.lock() {
                            *guard = Some(format!("Daemon snapshot stream unavailable: {error}"));
                        }
                    }
                }
                thread::sleep(Duration::from_secs(2));
            });
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            get_snapshot,
            get_pulse_score,
            get_home_summary,
            list_tweaks,
            apply_tweak,
            revert_tweak,
            apply_safe_tweaks,
            apply_high_impact_tweaks,
            revert_all_tweaks,
            get_optimization_presets,
            apply_optimization_preset,
            get_boost_up_actions,
            run_boost_up_action,
            quick_benchmark,
            full_benchmark,
            get_benchmark_history,
            get_system_advisor,
            get_system_advisor_summary,
            get_system_advisor_detail,
            detect_games,
            ask_agent,
            refresh_all,
            clean_junk,
            optimize_ram,
            optimize_disk,
            flush_dns,
            optimize_tcp,
            refresh_latency,
            get_network_diagnostics,
            get_ram_breakdown,
            open_file_location,
            delete_large_file,
            kill_process,
            suspend_process,
            resume_process,
            create_restore_point,
            enable_game_mode,
            disable_game_mode,
            optimize_game,
            launch_optimized_game,
            revert_game_optimization,
            full_scan,
            take_snapshot,
            run_security_scan,
            autofix_low_risk,
            fix_security_issue,
            find_duplicates,
            get_recent_actions,
            get_snapshots,
            get_backup_summary,
            restore_snapshot,
            export_health_report,
            scan_large_files,
            get_game_mode_status,
            estimate_junk,
            get_license_info,
            activate_license,
            get_ai_preferences,
            set_ai_preferences,
            check_for_updates,
            get_error_log,
            export_error_log,
            toggle_startup_item,
            delay_startup_item,
            disable_selected_startup,
            delay_selected_startup,
            get_scheduled_tasks,
            set_scheduled_task,
            flush_standby_list,
            enable_ram_saver
        ])
        .run(tauri::generate_context!())
        .expect("error while running PulseBoost Tauri UI");
}






