/*
 * hyperv_driver.c: core driver functions for managing Microsoft Hyper-V hosts
 *
 * Copyright (C) 2016 Datto Inc.
 * Copyright (C) 2011-2013 Matthias Bolte <matthias.bolte@googlemail.com>
 * Copyright (C) 2009 Michael Sievers <msievers83@googlemail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include <config.h>

#include "internal.h"
#include "datatypes.h"
#include "virdomainobjlist.h"
#include "virauth.h"
#include "viralloc.h"
#include "virlog.h"
#include "viruuid.h"
#include "hyperv_driver.h"
#include "hyperv_private.h"
#include "hyperv_util.h"
#include "hyperv_wmi.h"
#include "openwsman.h"
#include "virstring.h"

#include "hyperv_api_v1.h"
#include "hyperv_network_api_v1.h"
#include "hyperv_api_v2.h"
#include "hyperv_network_api_v2.h"

#define VIR_FROM_THIS VIR_FROM_HYPERV

VIR_LOG_INIT("hyperv.hyperv_driver");

/* Forward declarations */
static virHypervisorDriver hypervHypervisorDriver;
static virNetworkDriver hypervNetworkDriver;

static int
hypervSetupV1(virHypervisorDriverPtr d, virNetworkDriverPtr n,
        hypervPrivate *priv)
{
    int result = -1;

    /* Set up driver functions based on what API version the server uses. */
    d->connectGetType = hyperv1ConnectGetType; /* 0.9.5 */
    d->connectGetVersion = hyperv1ConnectGetVersion; /* TODO: get current version */
    d->connectGetHostname = hyperv1ConnectGetHostname; /* 0.9.5 */
    d->connectGetMaxVcpus = hyperv1ConnectGetMaxVcpus; /* TODO: get current version */
    d->nodeGetInfo = hyperv1NodeGetInfo; /* 0.9.5 */
    d->connectListDomains = hyperv1ConnectListDomains; /* 0.9.5 */
    d->connectNumOfDomains = hyperv1ConnectNumOfDomains; /* 0.9.5 */
    d->domainCreateXML = hyperv1DomainCreateXML; /* TODO: get current version */
    d->domainDefineXML = hyperv1DomainDefineXML; /* TODO: get current version */
    d->domainUndefine = hyperv1DomainUndefine; /* TODO: get current version */
    d->domainUndefineFlags = hyperv1DomainUndefineFlags; /* TODO: get current version */
    d->domainAttachDevice = hyperv1DomainAttachDevice; /* TODO: get current verison */
    d->domainAttachDeviceFlags = hyperv1DomainAttachDeviceFlags; /* TODO: get current version */
    d->connectListAllDomains = hyperv1ConnectListAllDomains; /* 0.10.2 */
    d->domainLookupByID = hyperv1DomainLookupByID; /* 0.9.5 */
    d->domainLookupByUUID = hyperv1DomainLookupByUUID; /* 0.9.5 */
    d->domainLookupByName = hyperv1DomainLookupByName; /* 0.9.5 */
    d->domainSuspend = hyperv1DomainSuspend; /* 0.9.5 */
    d->domainResume = hyperv1DomainResume; /* 0.9.5 */
    d->domainShutdown = hyperv1DomainShutdown; /* TODO: get current version */
    d->domainShutdownFlags = hyperv1DomainShutdownFlags; /* TODO: get current version */
    d->domainReboot = hyperv1DomainReboot; /* TODO: get current version */
    d->domainDestroy = hyperv1DomainDestroy; /* 0.9.5 */
    d->domainDestroyFlags = hyperv1DomainDestroyFlags; /* 0.9.5 */
    d->domainGetOSType = hyperv1DomainGetOSType; /* 0.9.5 */
    d->domainGetMaxMemory = hyperv1DomainGetMaxMemory; /* TODO: get current version */
    d->domainSetMaxMemory = hyperv1DomainSetMaxMemory; /* TODO: get current version */
    d->domainSetMemory = hyperv1DomainSetMemory; /* TODO: get current version */
    d->domainSetMemoryFlags = hyperv1DomainSetMemoryFlags; /* TODO: get current version */
    d->domainGetInfo = hyperv1DomainGetInfo; /* 0.9.5 */
    d->domainGetState = hyperv1DomainGetState; /* 0.9.5 */
    d->domainScreenshot = hyperv1DomainScreenshot; /* TODO: get current version */
    d->domainSetVcpus = hyperv1DomainSetVcpus; /* TODO: get current version */
    d->domainSetVcpusFlags = hyperv1DomainSetVcpusFlags; /* TODO: get current version */
    d->domainGetVcpusFlags = hyperv1DomainGetVcpusFlags; /* TODO: get current version */
    d->domainGetVcpus = hyperv1DomainGetVcpus; /* TODO: Get current version */
    d->domainGetMaxVcpus = hyperv1DomainGetMaxVcpus; /* TODO: get current version */
    d->domainGetXMLDesc = hyperv1DomainGetXMLDesc; /* 0.9.5 */
    d->connectListDefinedDomains = hyperv1ConnectListDefinedDomains; /* 0.9.5 */
    d->connectNumOfDefinedDomains = hyperv1ConnectNumOfDefinedDomains; /* 0.9.5 */
    d->domainCreate = hyperv1DomainCreate; /* 0.9.5 */
    d->domainCreateWithFlags = hyperv1DomainCreateWithFlags; /* 0.9.5 */
    d->domainGetAutostart = hyperv1DomainGetAutostart; /* TODO: get current version */
    d->domainSetAutostart = hyperv1DomainSetAutostart; /* TODO: get current version */
    d->domainGetSchedulerType = hyperv1DomainGetSchedulerType; /* TODO: get current version */
    d->domainGetSchedulerParameters = hyperv1DomainGetSchedulerParameters; /* TODO: get current version */
    d->domainGetSchedulerParametersFlags = hyperv1DomainGetSchedulerParametersFlags; /* TODO: get current version */
    d->nodeGetFreeMemory = hyperv1NodeGetFreeMemory; /* TODO: get current version */
    d->domainIsActive = hyperv1DomainIsActive;
    d->domainManagedSave = hyperv1DomainManagedSave; /* 0.9.5 */
    d->domainHasManagedSaveImage = hyperv1DomainHasManagedSaveImage; /* 0.9.5 */
    d->domainManagedSaveRemove = hyperv1DomainManagedSaveRemove; /* 0.9.5 */
    d->domainSendKey = hyperv1DomainSendKey; /* TODO: get current version */

    /* Set up network driver functions */
    n->connectListAllNetworks = hyperv1ConnectListAllNetworks; /* TODO: get current version */
    n->connectListNetworks = hyperv1ConnectListNetworks; /* TODO: get current version */
    n->connectNumOfNetworks = hyperv1ConnectNumOfNetworks; /* TODO: get current version */
    n->connectListDefinedNetworks = hyperv1ConnectListDefinedNetworks; /* TODO: get current version */
    n->networkLookupByName = hyperv1NetworkLookupByName; /* TODO: get current version */
    n->connectNumOfDefinedNetworks = hyperv1ConnectNumOfDefinedNetworks; /* TODO: get current version */

    /* set up capabilities */
    priv->caps = hyperv1CapsInit(priv);
    if (priv->caps == NULL)
        goto cleanup;

    result = 0;

cleanup:
    return result;
}

static int
hypervSetupV2(virHypervisorDriverPtr d, virNetworkDriverPtr n,
        hypervPrivate *priv)
{
    int result = -1;

    /* Set up driver functions based on what API version the server uses. */
    d->connectGetType = hyperv2ConnectGetType; /* TODO: Get current version */
    d->connectGetVersion = hyperv2ConnectGetVersion; /* TODO: get current version */
    d->connectGetHostname = hyperv2ConnectGetHostname; /* TODO: Get current version */
    d->connectGetMaxVcpus = hyperv2ConnectGetMaxVcpus; /* TODO: get current version */
    d->nodeGetInfo = hyperv2NodeGetInfo; /* TODO: Get current version */
    d->connectListDomains = hyperv2ConnectListDomains; /* TODO: Get current version */
    d->connectNumOfDomains = hyperv2ConnectNumOfDomains; /* TODO: Get current version */
    d->domainCreateXML = hyperv2DomainCreateXML; /* TODO: get current version */
    d->domainDefineXML = hyperv2DomainDefineXML; /* TODO: get current version */
    d->domainUndefine = hyperv2DomainUndefine; /* TODO: get current version */
    d->domainUndefineFlags = hyperv2DomainUndefineFlags; /* TODO: get current version */
    d->domainAttachDevice = hyperv2DomainAttachDevice; /* TODO: get current verison */
    d->domainAttachDeviceFlags = hyperv2DomainAttachDeviceFlags; /* TODO: get current version */
    d->connectListAllDomains = hyperv2ConnectListAllDomains; /* 0.10.2 */
    d->domainLookupByID = hyperv2DomainLookupByID; /* TODO: Get current version */
    d->domainLookupByUUID = hyperv2DomainLookupByUUID; /* TODO: Get current version */
    d->domainLookupByName = hyperv2DomainLookupByName; /* TODO: Get current version */
    d->domainSuspend = hyperv2DomainSuspend; /* TODO: Get current version */
    d->domainResume = hyperv2DomainResume; /* TODO: Get current version */
    d->domainShutdown = hyperv2DomainShutdown; /* TODO: get current version */
    d->domainShutdownFlags = hyperv2DomainShutdownFlags; /* TODO: get current version */
    d->domainReboot = hyperv2DomainReboot; /* TODO: get current version */
    d->domainDestroy = hyperv2DomainDestroy; /* TODO: Get current version */
    d->domainDestroyFlags = hyperv2DomainDestroyFlags; /* TODO: Get current version */
    d->domainGetOSType = hyperv2DomainGetOSType; /* TODO: Get current version */
    d->domainGetMaxMemory = hyperv2DomainGetMaxMemory; /* TODO: get current version */
    d->domainSetMaxMemory = hyperv2DomainSetMaxMemory; /* TODO: get current version */
    d->domainSetMemory = hyperv2DomainSetMemory; /* TODO: get current version */
    d->domainSetMemoryFlags = hyperv2DomainSetMemoryFlags; /* TODO: get current version */
    d->domainGetInfo = hyperv2DomainGetInfo; /* TODO: Get current version */
    d->domainGetState = hyperv2DomainGetState; /* TODO: Get current version */
    d->domainScreenshot = hyperv2DomainScreenshot; /* TODO: get current version */
    d->domainSetVcpus = hyperv2DomainSetVcpus; /* TODO: get current version */
    d->domainSetVcpusFlags = hyperv2DomainSetVcpusFlags; /* TODO: get current version */
    d->domainGetVcpusFlags = hyperv2DomainGetVcpusFlags; /* TODO: get current version */
    d->domainGetVcpus = hyperv2DomainGetVcpus; /* TODO: Get current version */
    d->domainGetMaxVcpus = hyperv2DomainGetMaxVcpus; /* TODO: get current version */
    d->domainGetXMLDesc = hyperv2DomainGetXMLDesc; /* TODO: Get current version */
    d->connectListDefinedDomains = hyperv2ConnectListDefinedDomains; /* TODO: Get current version */
    d->connectNumOfDefinedDomains = hyperv2ConnectNumOfDefinedDomains; /* TODO: Get current version */
    d->domainCreate = hyperv2DomainCreate; /* TODO: Get current version */
    d->domainCreateWithFlags = hyperv2DomainCreateWithFlags; /* TODO: Get current version */
    d->domainGetAutostart = hyperv2DomainGetAutostart; /* TODO: get current version */
    d->domainSetAutostart = hyperv2DomainSetAutostart; /* TODO: get current version */
    d->domainGetSchedulerType = hyperv2DomainGetSchedulerType; /* TODO: get current version */
    d->domainGetSchedulerParameters = hyperv2DomainGetSchedulerParameters; /* TODO: get current version */
    d->domainGetSchedulerParametersFlags = hyperv2DomainGetSchedulerParametersFlags; /* TODO: get current version */
    d->nodeGetFreeMemory = hyperv2NodeGetFreeMemory; /* TODO: get current version */
    d->domainIsActive = hyperv2DomainIsActive;
    d->domainManagedSave = hyperv2DomainManagedSave; /* TODO: Get current version */
    d->domainHasManagedSaveImage = hyperv2DomainHasManagedSaveImage; /* TODO: Get current version */
    d->domainManagedSaveRemove = hyperv2DomainManagedSaveRemove; /* TODO: Get current version */
    d->domainSendKey = hyperv2DomainSendKey; /* TODO: get current version */

    /* Set up network driver functions */
    n->connectListAllNetworks = hyperv2ConnectListAllNetworks; /* TODO: get current version */
    n->connectListNetworks = hyperv2ConnectListNetworks; /* TODO: get current version */
    n->connectNumOfNetworks = hyperv2ConnectNumOfNetworks; /* TODO: get current version */
    n->connectListDefinedNetworks = hyperv2ConnectListDefinedNetworks; /* TODO: get current version */
    n->networkLookupByName = hyperv2NetworkLookupByName; /* TODO: get current version */
    n->connectNumOfDefinedNetworks = hyperv2ConnectNumOfDefinedNetworks; /* TODO: get current version */

    /* set up capabilities */
    priv->caps = hyperv2CapsInit(priv);
    if (priv->caps == NULL)
        goto cleanup;

    result = 0;

cleanup:
    return result;
}

static char *
hypervNodeGetWindowsVersion(hypervPrivate *priv)
{
    virBuffer query = VIR_BUFFER_INITIALIZER;
    Win32_OperatingSystem *os = NULL;
    char *result = NULL;

    virBufferAddLit(&query, WIN32_OPERATINGSYSTEM_WQL_SELECT);

    if (hypervGetWin32OperatingSystemList(priv, &query, &os) < 0)
        goto cleanup;

    if (os == NULL) {
        virReportError(VIR_ERR_INTERNAL_ERROR, _("Could not get OS info"));
        goto cleanup;
    }

    result = os->data->Version;

cleanup:
    hypervFreeObject(priv, (hypervObject *) os);
    virBufferFreeAndReset(&query);
    return result;
}

static virDrvOpenStatus
hypervConnectOpen(virConnectPtr conn, virConnectAuthPtr auth,
                  virConfPtr conf ATTRIBUTE_UNUSED,
                  unsigned int flags)
{
    virDrvOpenStatus result = VIR_DRV_OPEN_ERROR;
    char *plus;
    hypervPrivate *priv = NULL;
    char *username = NULL;
    char *password = NULL;
    virBuffer query = VIR_BUFFER_INITIALIZER;
    Msvm_ComputerSystem_V1 *computerSystem = NULL;
    Msvm_ComputerSystem_V2 *computerSystem_v2 = NULL;
    char *winVersion = NULL;

    virCheckFlags(VIR_CONNECT_RO, VIR_DRV_OPEN_ERROR);

    /* Decline if the URI is NULL or the scheme is NULL */
    if (conn->uri == NULL || conn->uri->scheme == NULL)
        return VIR_DRV_OPEN_DECLINED;

    /* Decline if the scheme is not hyperv */
    plus = strchr(conn->uri->scheme, '+');

    if (plus == NULL) {
        if (STRCASENEQ(conn->uri->scheme, "hyperv"))
            return VIR_DRV_OPEN_DECLINED;
    } else {
        if (plus - conn->uri->scheme != 6 ||
            STRCASENEQLEN(conn->uri->scheme, "hyperv", 6)) {
            return VIR_DRV_OPEN_DECLINED;
        }

        virReportError(VIR_ERR_INVALID_ARG,
                       _("Transport '%s' in URI scheme is not supported, try again "
                         "without the transport part"), plus + 1);
        return VIR_DRV_OPEN_ERROR;
    }

    /* Require server part */
    if (conn->uri->server == NULL) {
        virReportError(VIR_ERR_INVALID_ARG, "%s",
                       _("URI is missing the server part"));
        return VIR_DRV_OPEN_ERROR;
    }

    /* Require auth */
    if (auth == NULL || auth->cb == NULL) {
        virReportError(VIR_ERR_INVALID_ARG, "%s",
                       _("Missing or invalid auth pointer"));
        return VIR_DRV_OPEN_ERROR;
    }

    /* Allocate per-connection private data */
    if (VIR_ALLOC(priv) < 0)
        goto cleanup;

    if (hypervParseUri(&priv->parsedUri, conn->uri) < 0)
        goto cleanup;

    /* Set the port dependent on the transport protocol if no port is
     * specified. This allows us to rely on the port parameter being
     * correctly set when building URIs later on, without the need to
     * distinguish between the situations port == 0 and port != 0 */
    if (conn->uri->port == 0) {
        if (STRCASEEQ(priv->parsedUri->transport, "https")) {
            conn->uri->port = 5986;
        } else {
            conn->uri->port = 5985;
        }
    }

    /* Request credentials */
    if (conn->uri->user != NULL) {
        if (VIR_STRDUP(username, conn->uri->user) < 0)
            goto cleanup;
    } else {
        username = virAuthGetUsername(conn, auth, "hyperv", "administrator", conn->uri->server);

        if (username == NULL) {
            virReportError(VIR_ERR_AUTH_FAILED, "%s", _("Username request failed"));
            goto cleanup;
        }
    }

    password = virAuthGetPassword(conn, auth, "hyperv", username, conn->uri->server);

    if (password == NULL) {
        virReportError(VIR_ERR_AUTH_FAILED, "%s", _("Password request failed"));
        goto cleanup;
    }

    /* Initialize the openwsman connection */
    priv->client = wsmc_create(conn->uri->server, conn->uri->port, "/wsman",
                               priv->parsedUri->transport, username, password);

    if (priv->client == NULL) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("Could not create openwsman client"));
        goto cleanup;
    }

    if (wsmc_transport_init(priv->client, NULL) != 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("Could not initialize openwsman transport"));
        goto cleanup;
    }

    /* FIXME: Currently only basic authentication is supported  */
    wsman_transport_set_auth_method(priv->client, "basic");

    /* init xmlopt for domain XML */
    priv->xmlopt = virDomainXMLOptionNew(NULL, NULL, NULL);

    /* determine what version of Windows we're dealing with */
    winVersion = hypervNodeGetWindowsVersion(priv);
    VIR_DEBUG("Windows version reported as '%s'", winVersion);
    if (!winVersion) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                _("Could not determine Windows version. Check username and password."));
        goto cleanup;
    }

    if (STRPREFIX(winVersion, HYPERV_VERSION_2008) ||
        STRPREFIX(winVersion, HYPERV_VERSION_2012)) {
        /* Check if the connection can be established and if the server has the
         * Hyper-V role installed. If the call to hyperv1GetMsvmComputerSystemList
         * succeeds than the connection has been established. If the returned list
         * is empty than the server isn't a Hyper-V server. */
        virBufferAddLit(&query, MSVM_COMPUTERSYSTEM_V1_WQL_SELECT);
        virBufferAddLit(&query, "where ");
        virBufferAddLit(&query, MSVM_COMPUTERSYSTEM_V1_WQL_PHYSICAL);

        if (hyperv1GetMsvmComputerSystemList(priv, &query, &computerSystem) < 0)
            goto cleanup;

        if (computerSystem == NULL) {
            virReportError(VIR_ERR_INTERNAL_ERROR,
                           _("%s is not a Hyper-V server"), conn->uri->server);
            goto cleanup;
        }

        hypervSetupV1(&hypervHypervisorDriver, &hypervNetworkDriver, priv);
    } else if (STRPREFIX(winVersion, HYPERV_VERSION_2012_R2) ||
               STRPREFIX(winVersion, HYPERV_VERSION_2016)) {
        /* Check if the connection can be established and if the server has the
         * Hyper-V role installed. If the call to hyperv2GetMsvmComputerSystemList
         * succeeds than the connection has been established. If the returned list
         * is empty than the server isn't a Hyper-V server. */
        virBufferAddLit(&query, MSVM_COMPUTERSYSTEM_V2_WQL_SELECT);
        virBufferAddLit(&query, "where ");
        virBufferAddLit(&query, MSVM_COMPUTERSYSTEM_V2_WQL_PHYSICAL);

        if (hyperv2GetMsvmComputerSystemList(priv, &query, &computerSystem_v2) < 0)
            goto cleanup;

        if (computerSystem_v2 == NULL) {
            virReportError(VIR_ERR_INTERNAL_ERROR,
                           _("%s is not a Hyper-V server"), conn->uri->server);
            goto cleanup;
        }

        hypervSetupV2(&hypervHypervisorDriver, &hypervNetworkDriver, priv);
    } else {
        // whatever this is, it's not supported
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s", _("Unsupported Windows version"));
        goto cleanup;
    }

    conn->privateData = priv;
    priv = NULL;
    result = VIR_DRV_OPEN_SUCCESS;

 cleanup:
    hypervFreePrivate(&priv);
    VIR_FREE(username);
    VIR_FREE(password);
    hypervFreeObject(priv, (hypervObject *)computerSystem);
    hypervFreeObject(priv, (hypervObject *)computerSystem_v2);

    return result;
}



static int
hypervConnectClose(virConnectPtr conn)
{
    hypervPrivate *priv = conn->privateData;

    hypervFreePrivate(&priv);

    conn->privateData = NULL;

    return 0;
}


static char*
hypervConnectGetCapabilities(virConnectPtr conn)
{
    hypervPrivate *priv = conn->privateData;
    char *xml = virCapabilitiesFormatXML(priv->caps);

    if (xml == NULL) {
        virReportOOMError();
        return NULL;
    }

    return xml;
}



static int
hypervConnectIsEncrypted(virConnectPtr conn)
{
    hypervPrivate *priv = conn->privateData;

    if (STRCASEEQ(priv->parsedUri->transport, "https")) {
        return 1;
    } else {
        return 0;
    }
}



static int
hypervConnectIsSecure(virConnectPtr conn)
{
    hypervPrivate *priv = conn->privateData;

    if (STRCASEEQ(priv->parsedUri->transport, "https")) {
        return 1;
    } else {
        return 0;
    }
}



static int
hypervConnectIsAlive(virConnectPtr conn)
{
    hypervPrivate *priv = conn->privateData;

    /* XXX we should be able to do something better than this is simple, safe,
     * and good enough for now. In worst case, the function will return true
     * even though the connection is not alive.
     */
    if (priv->client)
        return 1;
    else
        return 0;
}



static int
hypervDomainIsPersistent(virDomainPtr domain ATTRIBUTE_UNUSED)
{
    /* Hyper-V has no concept of transient domains, so all of them are persistent */
    return 1;
}



static int
hypervDomainIsUpdated(virDomainPtr domain ATTRIBUTE_UNUSED)
{
    return 0;
}



static virHypervisorDriver hypervHypervisorDriver = {
    .name = "Hyper-V",
    .connectOpen = hypervConnectOpen, /* 0.9.5 */
    .connectClose = hypervConnectClose, /* 0.9.5 */
    .connectIsAlive = hypervConnectIsAlive, /* 0.9.8 */
    .connectIsEncrypted = hypervConnectIsEncrypted, /* 0.9.5 */
    .connectIsSecure = hypervConnectIsSecure, /* 0.9.5 */
    .domainIsPersistent = hypervDomainIsPersistent, /* 0.9.5 */
    .domainIsUpdated = hypervDomainIsUpdated, /* 0.9.5 */
    .connectGetCapabilities = hypervConnectGetCapabilities, /* TODO: replace with newest release */
};



static void
hypervDebugHandler(const char *message, debug_level_e level,
                   void *user_data ATTRIBUTE_UNUSED)
{
    switch (level) {
      case DEBUG_LEVEL_ERROR:
      case DEBUG_LEVEL_CRITICAL:
        VIR_ERROR(_("openwsman error: %s"), message);
        break;

      case DEBUG_LEVEL_WARNING:
        VIR_WARN("openwsman warning: %s", message);
        break;

      default:
        /* Ignore the rest */
        break;
    }
}


static virConnectDriver hypervConnectDriver = {
    .hypervisorDriver = &hypervHypervisorDriver,
    .networkDriver = &hypervNetworkDriver,
};

int
hypervRegister(void)
{
    /* Forward openwsman errors and warnings to libvirt's logging */
    debug_add_handler(hypervDebugHandler, DEBUG_LEVEL_WARNING, NULL);

    return virRegisterConnectDriver(&hypervConnectDriver,
                                    false);
}
