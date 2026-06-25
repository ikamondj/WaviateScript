export async function clearMarketplaceTables() {
  return runAdminAction('/api/admin/clear');
}

export async function runMarketplaceSeeds() {
  return runAdminAction('/api/admin/seed');
}

async function runAdminAction(path) {
  const response = await fetch(path, {
    method: 'POST',
    headers: {
      Accept: 'application/json',
    },
  });
  const payload = await readPayload(response);

  if (!response.ok) {
    throw new Error(payload.error || `Admin action failed with HTTP ${response.status}`);
  }

  return payload;
}

async function readPayload(response) {
  try {
    return await response.json();
  } catch {
    return {};
  }
}
