# Spec Delta

## ADDED Requirements

### Requirement: Configurable runtime identity

The orchestration of the regeneration container SHALL let the operator determine the uid and gid that the
regeneration pass runs as, through the environment, defaulting to the identity the image documents (uid 1000,
gid 1000) when nothing is set. The mounted areas SHALL be written with that identity, so that a repository
mounted from a directory owned by the operator's own account is usable without changing ownership. The image
SHALL create its non-root user with the same documented numeric ids. The serving container SHALL keep the
identity of its base image.

#### Scenario: The default identity is the documented one

- **GIVEN** an orchestration run with no identity configured
- **WHEN** the regeneration pass starts
- **THEN** it SHALL run as uid 1000 and gid 1000
- **AND** the image's own non-root user SHALL have that same uid and gid

#### Scenario: The operator chooses the identity of the mounted areas

- **GIVEN** a repository area owned by an account other than 1000
- **WHEN** the operator configures that account's uid and gid and starts a pass
- **THEN** the pass SHALL run with that identity
- **AND** files it creates in the mounted areas SHALL carry that identity, needing no ownership change on
  the host side

#### Scenario: A configured identity still requires write access

- **GIVEN** a repository area that the configured identity may not write to
- **WHEN** a pass runs
- **THEN** it SHALL fail with the error of the underlying write, and SHALL NOT fall back to another identity

#### Scenario: The serving container is unaffected

- **GIVEN** the serving container
- **WHEN** it runs with an identity configured for the regeneration container
- **THEN** it SHALL keep the identity of its base image and continue to serve the repository read-only
