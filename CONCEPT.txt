Mimir

Cloud-Native Backup Orchestrator

Mimir is a cloud-native backup orchestration platform built around ephemeral workers, declarative policies, and object storage.

Instead of running permanent backup agents on every system, Mimir orchestrates short-lived workers that execute backup jobs on demand, transfer data to object storage, and terminate when finished.

Vision

Modern infrastructure has Kubernetes clusters, virtual machines, databases, network storage, and object storage. Existing backup solutions typically fall into one of two categories:

- Enterprise platforms with permanent agents and complex infrastructure
- Backup tools that provide storage and deduplication but lack orchestration

Mimir fills the gap between them.

It provides a control plane for backups while delegating data movement to ephemeral workers.

Architecture

BackupPolicy
      ↓
Backup Controller
      ↓
Job Scheduler
      ↓
Ephemeral Worker
      ↓
Snapshot / Read-Only Mount
      ↓
Restic / Kopia
      ↓
S3-Compatible Storage

Control Plane

The control plane is responsible for:

- Backup policies
- Scheduling
- Job lifecycle management
- Retention policies
- Monitoring and reporting
- Restore orchestration

The control plane never handles backup data directly.

Data Plane

The data plane consists of ephemeral workers.

A worker:

1. Receives a backup job
2. Attaches a snapshot or mounts a source volume in read-only mode
3. Executes a backup using Restic or Kopia
4. Uploads data to object storage
5. Reports status
6. Terminates

No permanent agents are required.

Core Principles

Declarative

Backups are described as desired state.

apiVersion: mimir.io/v1alpha1
kind: BackupPolicy

metadata:
  name: billing-db

spec:
  source:
    type: postgres

  schedule: "0 * * * *"

  destination:
    bucket: production-backups

  retention:
    hourly: 48
    daily: 30

Ephemeral Execution

Workers are created only when needed.

Create Job
    ↓
Run Backup
    ↓
Upload Data
    ↓
Destroy Worker

This minimizes resource consumption and reduces operational overhead.

Stateless Data Plane

Workers do not maintain state.

All metadata is stored by the controller and all backup data is stored in object storage.

S3-Native

Mimir treats object storage as the primary backup destination.

Supported targets include:

- Amazon S3
- Cloudflare R2
- Backblaze B2
- MinIO
- Storj
- Any S3-compatible implementation

Supported Sources

Filesystems

- Local disks
- Network shares
- NFS
- SMB

Kubernetes

- Persistent Volumes
- CSI VolumeSnapshots
- Namespace-level backups

Databases

- PostgreSQL
- MySQL
- MariaDB
- MongoDB

Virtual Machines

- VM snapshots
- Block storage volumes

Why Mimir

Traditional Backup Platforms| Mimir
Permanent agents| Ephemeral workers
Heavy infrastructure| Kubernetes-native
Complex deployment| Declarative configuration
Vendor lock-in| Open standards
Storage-specific workflows| S3-first architecture

Roadmap

Phase 1

- BackupPolicy CRD
- BackupJob CRD
- Kubernetes Operator
- Restic integration
- S3 support
- Basic monitoring

Phase 2

- Kopia support
- Restore workflows
- Retention automation
- Multi-cluster support

Phase 3

- Web UI
- RBAC
- Multi-tenant architecture
- Backup analytics
- Disaster recovery workflows

Non-Goals

Mimir is not:

- A backup engine
- A storage system
- A deduplication engine

Mimir orchestrates existing backup tools and infrastructure.

Name

In Norse mythology, Mimir is the keeper of wisdom and memory.

Just as Mimir preserves knowledge, the platform preserves the memory of your infrastructure.
