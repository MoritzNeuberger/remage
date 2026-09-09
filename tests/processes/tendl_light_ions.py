from __future__ import annotations

import pytest
from remage import remage_run

# TENDL only holds data for these five projectiles.
PARTICLES = ["proton", "deuteron", "triton", "He3", "alpha"]
# The TENDL data must reach the inelastic process of every hadronic physics option.
PHYSICS_LISTS = ["Shielding", "QGSP_BIC_HP", "QGSP_BERT_HP", "FTFP_BERT_HP"]


def _get_macro(physics_list, use_tendl) -> str:
    # /process/had/verbose makes Geant4 print the model table at the end of the run
    return f"""
/RMG/Manager/Logging/LogLevel error
/process/had/verbose 2
/RMG/Processes/HadronicPhysics {physics_list}
/RMG/Processes/EnableTENDLLightIons {str(use_tendl).lower()}
/run/initialize
/RMG/Generator/Confine UnConfined
/RMG/Generator/Select GPS
/gps/particle alpha
/gps/energy 5.3 MeV
/run/beamOn 1
"""


def _inelastic_models(capfd, physics_list, use_tendl):
    """Run one event and return the inelastic models of each particle."""
    remage_run(
        macros=_get_macro(physics_list, use_tendl),
        gdml_files="gdml/geometry.gdml",
    )
    output, _ = capfd.readouterr()

    models = {}
    particle = None
    is_inelastic = False
    for line in output.splitlines():
        text = line.strip()
        if text.startswith("Hadronic Processes for "):
            particle = text.removeprefix("Hadronic Processes for ")
            is_inelastic = False
        elif text.startswith("Process:"):
            is_inelastic = text.endswith("Inelastic")
        elif is_inelastic and text.startswith("Model:"):
            name = text.removeprefix("Model:").split(":")[0].strip()
            models.setdefault(particle, []).append(name)

    assert models, "Geant4 printed no process table"
    return models


@pytest.mark.parametrize("physics_list", PHYSICS_LISTS)
def test_tendl_is_used_for_all_physics_lists(capfd, physics_list):
    """ParticleHP must replace the default model of every hadronic physics option.

    Geant4 does this only in QGSP_BIC_AllHP, so remage sets it up itself.
    """
    models = _inelastic_models(capfd, physics_list, use_tendl=True)

    for particle in PARTICLES:
        result = models.get(particle, [])
        assert "ParticleHPInelastic" in result, (
            f"{particle} uses {result} instead of ParticleHP"
        )


def test_default_models_are_kept_without_the_command(capfd):
    """The command must be the only way to get ParticleHP for these particles."""
    models = _inelastic_models(capfd, PHYSICS_LISTS[0], use_tendl=False)

    for particle in PARTICLES:
        result = models.get(particle, [])
        assert "ParticleHPInelastic" not in result, (
            f"{particle} uses ParticleHP although it is disabled"
        )
